#ifndef SERVER_OP_REPO_H
#define SERVER_OP_REPO_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "db/Models.h"

namespace vec {

// Commits `transaction` and waits for the result, which is true if it was committed. Drogon only
// commits once the last reference to a transaction is gone, and doesn't wait for it, so never report a
// write as done without this. Pass the only reference, and don't call it after rollback() or a failed
// statement: Drogon never calls back then, so this would wait forever.
drogon::Task<bool> commit(std::shared_ptr<drogon::orm::Transaction> transaction);

// The result of OpRepo::insert.
struct InsertedOp {
    int64_t seq = 0;         // the op's place in the log; on a duplicate, the original's
    bool duplicate = false;  // this client_op_id was already in the log, so nothing changed
};

// Reads and writes the operations table. Unexpected database failures throw DrogonDbException.
class OpRepo {
public:
    static constexpr int maxOpsPerRead = 1000;

    explicit OpRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // Locks the document until `transaction` ends and returns its head_seq, which keeps writers on
    // every server instance in order. nullopt if there is no such document.
    static drogon::Task<std::optional<int64_t>> lockHead(
        const std::shared_ptr<drogon::orm::Transaction>& transaction, const std::string& documentId);

    // Appends an op at `headSeq` + 1 and advances the document's head. Call after lockHead(), with
    // what it returned, on the same transaction. The op is safe to retry until the transaction commits.
    static drogon::Task<InsertedOp> insert(const std::shared_ptr<drogon::orm::Transaction>& transaction,
                                           const std::string& documentId, int64_t headSeq,
                                           const std::string& userId, const std::string& clientOpId,
                                           const nlohmann::json& payload);

    // The seq of the op this client already sent with `clientOpId`, if any: a retry of an op that was
    // stored, perhaps before the connection dropped, should get its original ack.
    static drogon::Task<std::optional<int64_t>> seqOf(const drogon::orm::DbClientPtr& client,
                                                      const std::string& documentId,
                                                      const std::string& clientOpId);

    // Ops with seq > `seq`, oldest first, at most `limit` (clamped to 1..maxOpsPerRead).
    drogon::Task<std::vector<OpRow>> opsAfter(const std::string& documentId, int64_t seq,
                                              int limit = maxOpsPerRead);

    // Ops with seq < `before`, newest first, at most `limit` (clamped to 1..maxOpsPerRead), with the
    // email of the user who made each.
    drogon::Task<std::vector<HistoryRow>> history(const std::string& documentId, int64_t before, int limit);

private:
    drogon::orm::DbClientPtr database_;
};

// Reads and writes the snapshots table.
class SnapshotRepo {
public:
    explicit SnapshotRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // The newest snapshot taken at or before `atOrBefore`, if there is one.
    drogon::Task<std::optional<SnapshotRow>> latest(const std::string& documentId,
                                                    int64_t atOrBefore = std::numeric_limits<int64_t>::max());

    // False, changing nothing, if the document already has a snapshot at `seq`.
    drogon::Task<bool> insert(const std::string& documentId, int64_t seq, const nlohmann::json& state);

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_OP_REPO_H
