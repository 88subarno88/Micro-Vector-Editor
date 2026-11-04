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

        // Storing and applying the op come next in the pipeline.
        transaction->rollback();
        reject(sender, op, "not_implemented", "edits are not accepted yet");
    } catch (const std::exception& e) {
        vec::logError("op failed", {}, {{"document", room->documentId()}, {"error", e.what()}});
        reject(sender, op, "unavailable", "the document can't be edited right now");
    }
}
