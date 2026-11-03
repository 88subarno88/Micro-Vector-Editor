#include "db/DocRepo.h"

#include <drogon/orm/Result.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <string_view>

namespace vec {

namespace {

constexpr const char* columns =
    "id::text, owner_id::text, title, head_seq, to_char(created_at AT TIME ZONE 'UTC', "
    "'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"'), to_char(updated_at AT TIME ZONE 'UTC', "
    "'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"')";

Document toDocument(const drogon::orm::Row& row) {
    return {row[0].as<std::string>(), row[1].as<std::string>(), row[2].as<std::string>(),
            row[3].as<int64_t>(),     row[4].as<std::string>(), row[5].as<std::string>()};
}

// A cursor is "<created_at in microseconds>_<id>": the sort position of the last document on a page.
// It doesn't depend on that document still existing.
struct Cursor {
    int64_t createdAtMicros = 0;
    std::string id;
};

std::optional<Cursor> parseCursor(std::string_view text) {
    const size_t separator = text.find('_');
    if (separator == std::string_view::npos) return std::nullopt;

    const std::string_view micros = text.substr(0, separator);
    const std::string_view id = text.substr(separator + 1);
    Cursor cursor;
    const auto [end, error] =
        std::from_chars(micros.data(), micros.data() + micros.size(), cursor.createdAtMicros);
    if (error != std::errc() || end != micros.data() + micros.size() || !isUuid(id)) return std::nullopt;
    cursor.id = id;
    return cursor;
}

}  // namespace

drogon::Task<Document> DocRepo::create(const std::string& ownerId, const std::string& title) {
    const auto result = co_await database_->execSqlCoro(
        std::string(
            "WITH doc AS (INSERT INTO documents (owner_id, title) VALUES ($1::uuid, $2) RETURNING *), "
            "member AS (INSERT INTO document_members (document_id, user_id, role) "
            "           SELECT id, owner_id, 'owner' FROM doc) "
            "SELECT ") +
            columns + " FROM doc",
        ownerId, title);
    co_return toDocument(result[0]);
}

drogon::Task<std::optional<Document>> DocRepo::get(const std::string& id) {
    if (!isUuid(id)) co_return std::nullopt;
    const auto result = co_await database_->execSqlCoro(
        std::string("SELECT ") + columns + " FROM documents WHERE id = $1::uuid", id);
    if (result.empty()) co_return std::nullopt;
    co_return toDocument(result[0]);
}

drogon::Task<tl::expected<DocPage, ListDocsError>> DocRepo::list(const std::string& userId, int limit,
                                                                 const std::optional<std::string>& cursor) {
    const int pageSize = std::clamp(limit, 1, maxPageSize);
    const std::string select =
        std::string("SELECT ") + columns +
        ", (extract(epoch FROM created_at) * 1000000)::bigint "
        "FROM documents WHERE id IN (SELECT document_id FROM document_members WHERE user_id = $1::uuid) ";
    // Sorted by a column that never changes, so that editing a document can't move it between pages.
    constexpr const char* order = "ORDER BY created_at DESC, id DESC LIMIT ";
    const std::string fetch = std::to_string(pageSize + 1);  // one extra shows whether another page exists

    drogon::orm::Result result(nullptr);
    if (cursor) {
        const auto parsed = parseCursor(*cursor);
        if (!parsed) co_return tl::unexpected(ListDocsError::BadCursor);
        result = co_await database_->execSqlCoro(select +
                                                     "AND (created_at, id) < (TIMESTAMPTZ 'epoch' + "
                                                     "$2::bigint * INTERVAL '1 microsecond', $3::uuid) " +
                                                     order + fetch,
                                                 userId, parsed->createdAtMicros, parsed->id);
    } else {
        result = co_await database_->execSqlCoro(select + order + fetch, userId);
    }

    DocPage page;
    const size_t rows = std::min(result.size(), static_cast<size_t>(pageSize));
    for (size_t i = 0; i < rows; ++i) page.documents.push_back(toDocument(result[i]));
    if (result.size() > rows) {
        const auto last = result[rows - 1];
        page.nextCursor = std::to_string(last[6].as<int64_t>()) + "_" + last[0].as<std::string>();
    }
    co_return page;
}

drogon::Task<std::optional<Document>> DocRepo::rename(const std::string& id, const std::string& title) {
    if (!isUuid(id)) co_return std::nullopt;
    const auto result = co_await database_->execSqlCoro(
        std::string("UPDATE documents SET title = $2, updated_at = now() WHERE id = $1::uuid RETURNING ") +
            columns,
        id, title);
    if (result.empty()) co_return std::nullopt;
    co_return toDocument(result[0]);
}

drogon::Task<bool> DocRepo::remove(const std::string& id) {
    if (!isUuid(id)) co_return false;
    const auto result = co_await database_->execSqlCoro("DELETE FROM documents WHERE id = $1::uuid", id);
    co_return result.affectedRows() > 0;
}

drogon::Task<bool> MemberRepo::add(const std::string& documentId, const std::string& userId, Role role) {
    if (!isUuid(documentId) || !isUuid(userId)) co_return false;
    // Selecting from both tables, so that a missing row inserts nothing rather than raising an error.
    const auto result = co_await database_->execSqlCoro(
        "INSERT INTO document_members (document_id, user_id, role) "
        "SELECT d.id, u.id, $3 FROM documents d, users u WHERE d.id = $1::uuid AND u.id = $2::uuid "
        "ON CONFLICT (document_id, user_id) DO NOTHING",
        documentId, userId, std::string(toString(role)));
    co_return result.affectedRows() > 0;
}

drogon::Task<std::optional<Role>> MemberRepo::roleOf(const std::string& documentId,
                                                     const std::string& userId) {
    if (!isUuid(documentId) || !isUuid(userId)) co_return std::nullopt;
    const auto result = co_await database_->execSqlCoro(
        "SELECT role FROM document_members WHERE document_id = $1::uuid AND user_id = $2::uuid", documentId,
        userId);
    if (result.empty()) co_return std::nullopt;
    co_return roleFromString(result[0][0].as<std::string>());
}

}  // namespace vec
