#include "collab/OpPipeline.h"

#include <nlohmann/json.hpp>
#include <vector>

#include "db/OpRepo.h"
#include "logging/Log.h"
#include "ops/Apply.h"
#include "ops/Json.h"

using nlohmann::json;

namespace {

void reject(const OpSender& sender, const vec::OpMessage& op, const char* reason,
            const std::string& message) {
    sender.send(
        json{{"type", "reject"}, {"clientOpId", op.clientOpId}, {"reason", reason}, {"message", message}}
            .dump());
}

// An op as members receive it.
std::string opBroadcast(int64_t seq, const std::string& userId, const vec::Operation& op) {
    return json{{"type", "op"}, {"seq", seq}, {"userId", userId}, {"op", op}}.dump();
}

void ack(const OpSender& sender, const vec::OpMessage& op, int64_t seq) {
    sender.send(json{{"type", "ack"}, {"clientOpId", op.clientOpId}, {"seq", seq}}.dump());
}

// Brings the room up to `head` with the ops other instances committed. Resumes on the room's loop.
// False if a stored op couldn't be applied, which would mean the log and the room disagree.
drogon::Task<bool> catchUp(const drogon::orm::DbClientPtr& database, const std::shared_ptr<vec::Room>& room,
                           int64_t head) {
    while (room->lastSeq() < head) {
        const int64_t from = room->lastSeq();
        // Committed rows, so reading them outside the transaction is safe; the lock keeps new ones out.
        const auto rows =
            co_await vec::OpRepo(database).opsAfter(room->documentId(), from, static_cast<int>(head - from));
        co_await drogon::switchThreadCoro(room->loop());
        if (rows.empty()) co_return false;
        for (const vec::OpRow& row : rows) {
            const auto decoded = vec::decodeOperation(row.payload);
            if (!decoded || !room->applyCommitted(*decoded, row.seq)) co_return false;
            room->broadcast(opBroadcast(row.seq, row.userId, *decoded));
        }
        vec::logInfo("room caught up", {},
                     {{"document", room->documentId()}, {"ops", rows.size()}, {"lastSeq", room->lastSeq()}});
    }
    co_return true;
}

}  // namespace

drogon::Task<> processOp(drogon::orm::DbClientPtr database, std::shared_ptr<vec::Room> room, OpSender sender,
                         vec::OpMessage op) {
    try {
        auto transaction = co_await database->newTransactionCoro();
        const auto head = co_await vec::OpRepo::lockHead(transaction, room->documentId());
        co_await drogon::switchThreadCoro(room->loop());
        if (!head) {
            transaction->rollback();
            co_return reject(sender, op, "not_found", "the document no longer exists");
        }
        // A retry of an op already stored: answer as the first time, without applying it again. Checked
        // before the dry run, which would refuse a repeated addShape as a duplicate.
        if (const auto stored = co_await vec::OpRepo::seqOf(transaction, room->documentId(), op.clientOpId)) {
            transaction->rollback();
            co_return ack(sender, op, *stored);
        }
        co_await drogon::switchThreadCoro(room->loop());
        if (!co_await catchUp(database, room, *head)) {
            transaction->rollback();
            vec::logError("room catch-up failed", {}, {{"document", room->documentId()}, {"head", *head}});
            co_return reject(sender, op, "unavailable", "the document can't be edited right now");
        }

        // A dry run against the room's drawing, which is now up to date with the log.
        if (const auto checked = vec::check(room->diagram(), op.op); !checked) {
            transaction->rollback();
            co_return reject(sender, op, vec::toString(checked.error()),
                             "the edit doesn't apply to the drawing");
        }

        // ON CONFLICT DO NOTHING in insert() would also catch a retry that slipped past seqOf(); the row
        // lock makes that impossible, but it keeps the log safe regardless.
        const auto inserted = co_await vec::OpRepo::insert(
            transaction, room->documentId(), *head, sender.userId, op.clientOpId, nlohmann::json(op.op));
        if (!co_await vec::commit(std::move(transaction))) {
            co_await drogon::switchThreadCoro(room->loop());
            co_return reject(sender, op, "unavailable", "the edit couldn't be saved");
        }
        co_await drogon::switchThreadCoro(room->loop());
        if (inserted.duplicate) co_return ack(sender, op, inserted.seq);

        // Only now, with the op committed, does the room change and the sender hear back.
        if (!room->applyCommitted(op.op, inserted.seq)) {
            vec::logError("committed op didn't apply to the room", {},
                          {{"document", room->documentId()}, {"seq", inserted.seq}});
        }
        ack(sender, op, inserted.seq);
        room->broadcast(opBroadcast(inserted.seq, sender.userId, op.op), sender.session);
    } catch (const std::exception& e) {
        vec::logError("op failed", {}, {{"document", room->documentId()}, {"error", e.what()}});
        reject(sender, op, "unavailable", "the document can't be edited right now");
    }
}
