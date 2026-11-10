#include "collab/RemoteOps.h"

#include <nlohmann/json.hpp>

#include "collab/OpPipeline.h"
#include "db/OpRepo.h"
#include "logging/Log.h"
#include "ops/Json.h"

using nlohmann::json;

namespace vec {

namespace {

// Applies and broadcasts one committed op. False if it isn't the next one or doesn't apply, which would
// mean the room and the log disagree.
bool applyAndBroadcast(Room& room, int64_t seq, const std::string& userId, const std::string& clientOpId,
                       const Operation& op) {
    if (!room.applyCommitted(op, seq, userId, clientOpId)) return false;
    room.broadcast(opBroadcast(seq, userId, clientOpId, op));
    return true;
}

}  // namespace

drogon::Task<> applyRemoteOp(drogon::orm::DbClientPtr database, std::shared_ptr<Room> room,
                             std::string message, std::string instanceId) {
    try {
        const json parsed = json::parse(message, nullptr, false);
        if (!parsed.is_object() || !parsed.contains("seq") || !parsed["seq"].is_number_integer()) co_return;
        if (parsed.value("instance", "") == instanceId) co_return;  // our own op, applied when committed

        co_await drogon::switchThreadCoro(room->loop());
        const int64_t seq = parsed["seq"].get<int64_t>();
        if (room->loadState() != Room::LoadState::Loaded || seq <= room->lastSeq()) co_return;

        if (seq == room->lastSeq() + 1) {
            const auto op = decodeOperation(parsed.value("op", json()));
            if (op && applyAndBroadcast(*room, seq, parsed.value("userId", ""),
                                        parsed.value("clientOpId", ""), *op))
                co_return;
            // A message that doesn't apply as sent: fall through and trust the log instead.
        }

        // A gap: the ops in between went missing or haven't arrived yet. Postgres has them all.
        while (room->lastSeq() < seq) {
            const int64_t from = room->lastSeq();
            const auto rows =
                co_await OpRepo(database).opsAfter(room->documentId(), from, static_cast<int>(seq - from));
            co_await drogon::switchThreadCoro(room->loop());
            if (rows.empty()) break;
            for (const OpRow& row : rows) {
                if (row.seq <= room->lastSeq()) continue;  // applied meanwhile
                const auto op = decodeOperation(row.payload);
                if (!op || !applyAndBroadcast(*room, row.seq, row.userId, row.clientOpId, *op)) {
                    logError("remote op didn't apply", {},
                             {{"document", room->documentId()}, {"seq", row.seq}});
                    co_return;
                }
            }
        }
    } catch (const std::exception& e) {
        logError("remote op failed", {}, {{"document", room->documentId()}, {"error", e.what()}});
    }
}

}  // namespace vec
