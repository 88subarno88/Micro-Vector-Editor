#ifndef SERVER_SNAPSHOTS_H
#define SERVER_SNAPSHOTS_H

#include <drogon/orm/DbClient.h>

#include <cstdint>
#include <memory>

#include "collab/Room.h"

namespace vec {

// A document gets a new snapshot once this many ops have passed since its last one, so that loading it
// replays at most this many.
constexpr int64_t snapshotInterval = 200;

// Saves the room's drawing as a snapshot if snapshotInterval ops have passed since its newest one. Call on
// the room's loop. The drawing is copied there, and written to the database in the background, so the
// room's next op isn't kept waiting; a failed write is logged and simply retried after the next op.
void snapshotIfDue(const drogon::orm::DbClientPtr& database, const std::shared_ptr<Room>& room);

}  // namespace vec

#endif  // SERVER_SNAPSHOTS_H
