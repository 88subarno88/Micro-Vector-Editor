#ifndef OP_PIPELINE_H
#define OP_PIPELINE_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <functional>
#include <memory>
#include <string>

#include "collab/Messages.h"
#include "collab/Room.h"

// Who sent an op, and how to answer them.
struct OpSender {
    std::string userId;
    std::function<void(const std::string&)> send;
};

// Handles one op for `room`. Run it as one of the room's tasks, so that ops never overlap: each locks
// the document's row in a transaction, which also keeps ops from other server instances in order, and
// first applies any ops those instances committed that this room hasn't seen. Never throws.
drogon::Task<> processOp(drogon::orm::DbClientPtr database, std::shared_ptr<vec::Room> room, OpSender sender,
                         vec::OpMessage op);

#endif  // OP_PIPELINE_H
