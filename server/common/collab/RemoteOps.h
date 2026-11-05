#ifndef SERVER_REMOTE_OPS_H
#define SERVER_REMOTE_OPS_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <memory>
#include <string>

#include "collab/Room.h"

namespace vec {

// Brings `room` up to date with an op another server instance committed and published (see OpPublisher):
// an op this room already has is ignored, as is this instance's own message (`instanceId`); the next op
// is applied; and if ops are missing in between, perhaps because messages arrived out of order, they are
// read from Postgres first. Each applied op is broadcast to the room's members. Run it as one of the
// room's tasks, so that it can't interleave with an op this instance is committing. Never throws.
drogon::Task<> applyRemoteOp(drogon::orm::DbClientPtr database, std::shared_ptr<Room> room,
                             std::string message, std::string instanceId);

}  // namespace vec

#endif  // SERVER_REMOTE_OPS_H
