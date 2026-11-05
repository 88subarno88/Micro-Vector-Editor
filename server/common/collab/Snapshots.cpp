#include "collab/Snapshots.h"

#include <drogon/utils/coroutine.h>

#include <exception>
#include <string>

#include "db/OpRepo.h"
#include "logging/Log.h"
#include "ops/Json.h"

namespace vec {

void snapshotIfDue(const drogon::orm::DbClientPtr& database, const std::shared_ptr<Room>& room) {
    const int64_t seq = room->lastSeq();
    const int64_t previous = room->snapshotSeq();
    if (seq - previous < snapshotInterval) return;

    room->setSnapshotSeq(seq);  // now, so the next op doesn't start a second one while this one is written
    drogon::async_run(
        [database, room, seq, previous, state = diagramToJson(room->diagram())]() -> drogon::Task<> {
            try {
                co_await SnapshotRepo(database).insert(room->documentId(), seq, state);
                logInfo("saved snapshot", {}, {{"document", room->documentId()}, {"seq", seq}});
            } catch (const std::exception& e) {
                logError("snapshot failed", {},
                         {{"document", room->documentId()}, {"seq", seq}, {"error", e.what()}});
                // Due again on the room's next op. Only if nothing newer was recorded meanwhile.
                room->loop()->queueInLoop([room, seq, previous] {
                    if (room->snapshotSeq() == seq) room->setSnapshotSeq(previous);
                });
            }
        });
}

}  // namespace vec
