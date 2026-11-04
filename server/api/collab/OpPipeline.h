#ifndef OP_PIPELINE_H
#define OP_PIPELINE_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "collab/Messages.h"
#include "collab/Room.h"

// Who sent an op, and how to answer them.
struct OpSender {
    vec::SessionId session = 0;
    std::string userId;
    std::function<void(const std::string&)> send;
};

// Handles one op for `room`, answering the sender with {"type": "ack", "clientOpId", "seq"} once it is
// committed, or {"type": "reject", "clientOpId", "reason", "message"}. Every other member of the room gets
// {"type": "op", "seq", "userId", "op"}, as they do for ops caught up from other instances; each member
// receives ops in seq order. Run it as one of the room's tasks,
// so that ops never overlap: each locks the document's row in a transaction, which also keeps ops from
// other server instances in order, and first applies any ops those instances committed that this room
// hasn't seen. A retry of a stored op (same clientOpId) gets the original ack. Never throws.
// Logs "op timing" with the microseconds spent in each stage, from `queuedAt`, when the op entered the
// room's queue.
drogon::Task<> processOp(drogon::orm::DbClientPtr database, std::shared_ptr<vec::Room> room, OpSender sender,
                         vec::OpMessage op, std::chrono::steady_clock::time_point queuedAt);

#endif  // OP_PIPELINE_H
