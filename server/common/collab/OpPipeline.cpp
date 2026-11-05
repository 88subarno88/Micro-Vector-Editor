#include "collab/OpPipeline.h"

#include <nlohmann/json.hpp>
#include <vector>

#include "collab/Snapshots.h"
#include "db/OpRepo.h"
#include "logging/Log.h"
#include "metrics/StageTimer.h"
#include "ops/Apply.h"
#include "ops/Json.h"

using nlohmann::json;

namespace vec {

namespace {

void reject(const OpSender& sender, const vec::OpMessage& op, const char* reason,
            const std::string& message) {
    sender.send(
        json{{"type", "reject"}, {"clientOpId", op.clientOpId}, {"reason", reason}, {"message", message}}
            .dump());
}

void ack(const OpSender& sender, const vec::OpMessage& op, int64_t seq) {
    sender.send(json{{"type", "ack"}, {"clientOpId", op.clientOpId}, {"seq", seq}}.dump());
}

// Brings the room up to `head` with the ops other instances committed. Resumes on the room's loop.
// False if a stored op couldn't be applied, which would mean the log and the room disagree.
drogon::Task<bool> catchUp(const std::shared_ptr<drogon::orm::Transaction>& transaction,
                           const std::shared_ptr<vec::Room>& room, int64_t head) {
    while (room->lastSeq() < head) {
        const int64_t from = room->lastSeq();
        // On the transaction's own connection: waiting for a second one while holding the first would
        // deadlock once every connection in the pool is held by an op doing the same.
        const auto rows = co_await vec::OpRepo(transaction)
                              .opsAfter(room->documentId(), from, static_cast<int>(head - from));
        co_await drogon::switchThreadCoro(room->loop());
        if (rows.empty()) co_return false;
        for (const vec::OpRow& row : rows) {
            const auto decoded = vec::decodeOperation(row.payload);
            if (!decoded || !room->applyCommitted(*decoded, row.seq, row.userId)) co_return false;
            room->broadcast(opBroadcast(row.seq, row.userId, *decoded));
        }
        vec::logInfo("room caught up", {},
                     {{"document", room->documentId()}, {"ops", rows.size()}, {"lastSeq", room->lastSeq()}});
    }
    co_return true;
}

}  // namespace

std::string opBroadcast(int64_t seq, const std::string& userId, const Operation& op) {
    return json{{"type", "op"}, {"seq", seq}, {"userId", userId}, {"op", op}}.dump();
}

drogon::Task<> processOp(drogon::orm::DbClientPtr database, std::shared_ptr<vec::Room> room, OpSender sender,
                         vec::OpMessage op, std::chrono::steady_clock::time_point queuedAt,
                         OpPublisher publish) {
    // One log line per op, with how long each stage took and how it ended, logged however it ends.
    struct Timing {
        vec::StageTimer timer;
        std::string documentId;
        std::string outcome = "unknown";
        ~Timing() {
            vec::logInfo("op timing", {},
                         {{"document", documentId}, {"outcome", outcome}, {"us", timer.toJson()}});
        }
    } timing{vec::StageTimer(queuedAt), room->documentId()};
    timing.timer.mark("queued");
    const auto rejectAs = [&](const char* reason, const std::string& message) {
        timing.outcome = std::string("reject:") + reason;
        reject(sender, op, reason, message);
    };

    try {
        auto transaction = co_await database->newTransactionCoro();
        const auto head = co_await vec::OpRepo::lockHead(transaction, room->documentId());
        timing.timer.mark("lock");
        co_await drogon::switchThreadCoro(room->loop());
        if (!head) {
            transaction->rollback();
            co_return rejectAs("not_found", "the document no longer exists");
        }
        // A retry of an op already stored: answer as the first time, without applying it again. Checked
        // before the dry run, which would refuse a repeated addShape as a duplicate.
        if (const auto stored = co_await vec::OpRepo::seqOf(transaction, room->documentId(), op.clientOpId)) {
            transaction->rollback();
            timing.outcome = "retry";
            co_return ack(sender, op, *stored);
        }
        timing.timer.mark("retry_check");
        co_await drogon::switchThreadCoro(room->loop());
        if (!co_await catchUp(transaction, room, *head)) {
            transaction->rollback();
            vec::logError("room catch-up failed", {}, {{"document", room->documentId()}, {"head", *head}});
            co_return rejectAs("unavailable", "the document can't be edited right now");
        }
        timing.timer.mark("catch_up");

        // A dry run against the room's drawing, which is now up to date with the log.
        if (const auto checked = vec::check(room->diagram(), op.op); !checked) {
            transaction->rollback();
            co_return rejectAs(vec::toString(checked.error()), "the edit doesn't apply to the drawing");
        }
        timing.timer.mark("check");

        // ON CONFLICT DO NOTHING in insert() would also catch a retry that slipped past seqOf(); the row
        // lock makes that impossible, but it keeps the log safe regardless.
        const auto inserted = co_await vec::OpRepo::insert(
            transaction, room->documentId(), *head, sender.userId, op.clientOpId, nlohmann::json(op.op));
        timing.timer.mark("insert");
        const bool committed = co_await vec::commit(std::move(transaction));
        timing.timer.mark("commit");
        co_await drogon::switchThreadCoro(room->loop());
        if (!committed) co_return rejectAs("unavailable", "the edit couldn't be saved");
        if (inserted.duplicate) {
            timing.outcome = "retry";
            co_return ack(sender, op, inserted.seq);
        }

        // Only now, with the op committed, does the room change and the sender hear back.
        if (!room->applyCommitted(op.op, inserted.seq, sender.userId)) {
            vec::logError("committed op didn't apply to the room", {},
                          {{"document", room->documentId()}, {"seq", inserted.seq}});
        }
        ack(sender, op, inserted.seq);
        room->broadcast(opBroadcast(inserted.seq, sender.userId, op.op), sender.session);
        // After the commit, like the ack: other instances only ever hear of ops that are in the log.
        if (publish)
            publish(room->documentId(),
                    json{{"seq", inserted.seq}, {"userId", sender.userId}, {"op", op.op}}.dump());
        snapshotIfDue(database, room);
        timing.timer.mark("apply_and_send");
        timing.outcome = "ack";
    } catch (const std::exception& e) {
        vec::logError("op failed", {}, {{"document", room->documentId()}, {"error", e.what()}});
        rejectAs("unavailable", "the document can't be edited right now");
    }
}

}  // namespace vec
