#include "db/OpRepo.h"

#include <drogon/orm/Result.h>

#include <algorithm>
#include <coroutine>
#include <utility>

using nlohmann::json;

namespace vec {

namespace {

class CommitAwaiter : public drogon::CallbackAwaiter<bool> {
public:
    explicit CommitAwaiter(std::shared_ptr<drogon::orm::Transaction> transaction)
        : transaction_(std::move(transaction)) {}

    void await_suspend(std::coroutine_handle<> handle) {
        transaction_->setCommitCallback([this, handle](bool committed) {
            setValue(committed);
            handle.resume();
        });
        transaction_.reset();  // this is what starts the commit
    }

private:
    std::shared_ptr<drogon::orm::Transaction> transaction_;
};

}  // namespace

drogon::Task<bool> commit(std::shared_ptr<drogon::orm::Transaction> transaction) {
    co_return co_await CommitAwaiter(std::move(transaction));
}

drogon::Task<std::optional<int64_t>> OpRepo::lockHead(
    const std::shared_ptr<drogon::orm::Transaction>& transaction, const std::string& documentId) {
    if (!isUuid(documentId)) co_return std::nullopt;
    const auto result = co_await transaction->execSqlCoro(
        "SELECT head_seq FROM documents WHERE id = $1::uuid FOR UPDATE", documentId);
    if (result.empty()) co_return std::nullopt;
    co_return result[0][0].as<int64_t>();
}

drogon::Task<InsertedOp> OpRepo::insert(const std::shared_ptr<drogon::orm::Transaction>& transaction,
                                        const std::string& documentId, int64_t headSeq,
                                        const std::string& userId, const std::string& clientOpId,
                                        const json& payload) {
    const int64_t seq = headSeq + 1;
    const auto inserted = co_await transaction->execSqlCoro(
        "INSERT INTO operations (document_id, seq, user_id, client_op_id, payload) "
        "VALUES ($1::uuid, $2::bigint, $3::uuid, $4, $5::jsonb) "
        "ON CONFLICT (document_id, client_op_id) DO NOTHING",
        documentId, seq, userId, clientOpId, payload.dump());

    if (inserted.affectedRows() == 0) {
        const auto original = co_await transaction->execSqlCoro(
            "SELECT seq FROM operations WHERE document_id = $1::uuid AND client_op_id = $2", documentId,
            clientOpId);
        co_return InsertedOp{original[0][0].as<int64_t>(), true};
    }

    co_await transaction->execSqlCoro(
        "UPDATE documents SET head_seq = $2::bigint, updated_at = now() WHERE id = $1::uuid", documentId,
        seq);
    co_return InsertedOp{seq, false};
}

drogon::Task<std::optional<int64_t>> OpRepo::seqOf(const drogon::orm::DbClientPtr& client,
                                                   const std::string& documentId,
                                                   const std::string& clientOpId) {
    if (!isUuid(documentId)) co_return std::nullopt;
    const auto result = co_await client->execSqlCoro(
        "SELECT seq FROM operations WHERE document_id = $1::uuid AND client_op_id = $2", documentId,
        clientOpId);
    if (result.empty()) co_return std::nullopt;
    co_return result[0][0].as<int64_t>();
}

drogon::Task<std::vector<OpRow>> OpRepo::opsAfter(const std::string& documentId, int64_t seq, int limit) {
    if (!isUuid(documentId)) co_return std::vector<OpRow>{};
    const auto result = co_await database_->execSqlCoro(
        "SELECT document_id::text, seq, user_id::text, client_op_id, payload::text, to_char(created_at AT "
        "TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"') "
        "FROM operations WHERE document_id = $1::uuid AND seq > $2::bigint ORDER BY seq LIMIT " +
            std::to_string(std::clamp(limit, 1, maxOpsPerRead)),
        documentId, seq);

    std::vector<OpRow> ops;
    ops.reserve(result.size());
    for (const auto& row : result) {
        ops.push_back({row[0].as<std::string>(), row[1].as<int64_t>(), row[2].as<std::string>(),
                       row[3].as<std::string>(), json::parse(row[4].as<std::string>()),
                       row[5].as<std::string>()});
    }
    co_return ops;
}

drogon::Task<std::vector<HistoryRow>> OpRepo::history(const std::string& documentId, int64_t before,
                                                      int limit) {
    if (!isUuid(documentId)) co_return std::vector<HistoryRow>{};
    const auto result = co_await database_->execSqlCoro(
        "SELECT o.seq, o.user_id::text, u.email, o.payload::text, to_char(o.created_at AT TIME ZONE 'UTC', "
        "'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"') "
        "FROM operations o JOIN users u ON u.id = o.user_id "
        "WHERE o.document_id = $1::uuid AND o.seq < $2::bigint ORDER BY o.seq DESC LIMIT " +
            std::to_string(std::clamp(limit, 1, maxOpsPerRead)),
        documentId, before);

    std::vector<HistoryRow> ops;
    ops.reserve(result.size());
    for (const auto& row : result) {
        ops.push_back({row[0].as<int64_t>(), row[1].as<std::string>(), row[2].as<std::string>(),
                       json::parse(row[3].as<std::string>()), row[4].as<std::string>()});
    }
    co_return ops;
}

drogon::Task<std::optional<SnapshotRow>> SnapshotRepo::latest(const std::string& documentId) {
    if (!isUuid(documentId)) co_return std::nullopt;
    const auto result = co_await database_->execSqlCoro(
        "SELECT document_id::text, seq, state::text, to_char(created_at AT TIME ZONE 'UTC', "
        "'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"') FROM snapshots "
        "WHERE document_id = $1::uuid ORDER BY seq DESC LIMIT 1",
        documentId);
    if (result.empty()) co_return std::nullopt;
    const auto row = result[0];
    co_return SnapshotRow{row[0].as<std::string>(), row[1].as<int64_t>(),
                          json::parse(row[2].as<std::string>()), row[3].as<std::string>()};
}

drogon::Task<bool> SnapshotRepo::insert(const std::string& documentId, int64_t seq, const json& state) {
    const auto result = co_await database_->execSqlCoro(
        "INSERT INTO snapshots (document_id, seq, state) VALUES ($1::uuid, $2::bigint, $3::jsonb) "
        "ON CONFLICT (document_id, seq) DO NOTHING",
        documentId, seq, state.dump());
    co_return result.affectedRows() > 0;
}

}  // namespace vec
