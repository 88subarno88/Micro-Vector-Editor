#include "db/ExportRepo.h"

#include <drogon/orm/Result.h>

namespace vec {

namespace {

constexpr const char* columns =
    "id::text, document_id::text, format, status, attempts, result_url, error, "
    "to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"')";

std::optional<std::string> optionalText(const drogon::orm::Field& field) {
    if (field.isNull()) return std::nullopt;
    return field.as<std::string>();
}

ExportJob toExportJob(const drogon::orm::Row& row) {
    return {row[0].as<std::string>(), row[1].as<std::string>(), row[2].as<std::string>(),
            row[3].as<std::string>(), row[4].as<int>(),         optionalText(row[5]),
            optionalText(row[6]),     row[7].as<std::string>()};
}

}  // namespace

bool isExportFormat(std::string_view format) { return format == "svg" || format == "png"; }

drogon::Task<ExportJob> ExportRepo::enqueue(const std::string& documentId, const std::string& format) {
    // Two requests at the same moment can both insert; that only costs one extra export.
    const auto result = co_await database_->execSqlCoro(
        std::string("WITH queued AS (SELECT * FROM export_jobs "
                    "                WHERE document_id = $1::uuid AND format = $2 AND status = 'queued' "
                    "                ORDER BY created_at LIMIT 1), "
                    "inserted AS (INSERT INTO export_jobs (document_id, format, status) "
                    "             SELECT $1::uuid, $2, 'queued' WHERE NOT EXISTS (SELECT 1 FROM queued) "
                    "             RETURNING *) "
                    "SELECT ") +
            columns + " FROM queued UNION ALL SELECT " + columns + " FROM inserted",
        documentId, format);
    co_return toExportJob(result[0]);
}

drogon::Task<std::optional<ExportJob>> ExportRepo::get(const std::string& id) {
    if (!isUuid(id)) co_return std::nullopt;
    const auto result = co_await database_->execSqlCoro(
        std::string("SELECT ") + columns + " FROM export_jobs WHERE id = $1::uuid", id);
    if (result.empty()) co_return std::nullopt;
    co_return toExportJob(result[0]);
}

drogon::Task<std::optional<ExportJob>> ExportRepo::claim() {
    const auto result = co_await database_->execSqlCoro(
        std::string("UPDATE export_jobs SET status = 'running', attempts = attempts + 1 "
                    "WHERE id = (SELECT id FROM export_jobs WHERE status = 'queued' AND run_after <= now() "
                    "            ORDER BY created_at FOR UPDATE SKIP LOCKED LIMIT 1) "
                    "RETURNING ") +
        columns);
    if (result.empty()) co_return std::nullopt;
    co_return toExportJob(result[0]);
}

drogon::Task<bool> ExportRepo::finish(const std::string& id, const std::string& resultUrl) {
    const auto result = co_await database_->execSqlCoro(
        "UPDATE export_jobs SET status = 'done', result_url = $2 WHERE id = $1::uuid AND status = 'running'",
        id, resultUrl);
    co_return result.affectedRows() == 1;
}

drogon::Task<bool> ExportRepo::fail(const std::string& id, const std::string& error) {
    const auto result = co_await database_->execSqlCoro(
        "UPDATE export_jobs SET status = 'failed', error = $2 WHERE id = $1::uuid AND status = 'running'", id,
        error);
    co_return result.affectedRows() == 1;
}

}  // namespace vec
