#include "DocRoutes.h"

#include <drogon/drogon.h>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <tl/expected.hpp>

#include "ApiResponse.h"
#include "AuthFilter.h"
#include "Routes.h"
#include "db/DocRepo.h"
#include "db/OpRepo.h"
#include "docs/Title.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using nlohmann::json;

namespace {

constexpr int defaultPageSize = 20;

json toJson(const vec::Document& doc) {
    return {{"id", doc.id},           {"ownerId", doc.ownerId},     {"title", doc.title},
            {"headSeq", doc.headSeq}, {"createdAt", doc.createdAt}, {"updatedAt", doc.updatedAt}};
}

// The title from a {"title": ...} body, normalised; otherwise the 400 response to send.
tl::expected<std::string, HttpResponsePtr> titleFrom(const HttpRequestPtr& request) {
    const json body = json::parse(request->body(), nullptr, false);
    const auto field = body.is_object() ? body.find("title") : body.end();
    if (field == body.end() || !field->is_string()) {
        return tl::unexpected(errorResponse(drogon::k400BadRequest, "bad_request",
                                            "expected a JSON object with a string field title"));
    }
    const auto title = vec::normalizeTitle(field->get<std::string>());
    if (!title) {
        return tl::unexpected(errorResponse(drogon::k400BadRequest, "invalid_title",
                                            "title must be 1 to 200 bytes, with no control characters"));
    }
    return *title;
}

HttpResponsePtr notFound() { return errorResponse(drogon::k404NotFound, "not_found", "no such document"); }

// The whole of `text` as an int; nullopt if anything else is in it.
std::optional<int> parseInt(const std::string& text) {
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size()) return std::nullopt;
    return value;
}

}  // namespace

void registerDocRoutes(const drogon::orm::DbClientPtr& database) {
    registerProtected(
        "/api/docs",
        [database](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const auto title = titleFrom(request);
            if (!title) co_return title.error();

            const vec::Document doc = co_await vec::DocRepo(database).create(userIdOf(request), *title);
            co_return jsonResponse(drogon::k201Created, toJson(doc));
        },
        drogon::Post);

    registerProtected(
        "/api/docs",
        [database](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const std::string& limitText = request->getParameter("limit");
            const auto limit = limitText.empty() ? std::optional<int>(defaultPageSize) : parseInt(limitText);
            if (!limit)
                co_return errorResponse(drogon::k400BadRequest, "bad_request", "limit must be a number");

            const std::string& cursorText = request->getParameter("cursor");
            const auto cursor = cursorText.empty() ? std::nullopt : std::optional<std::string>(cursorText);
            const auto page = co_await vec::DocRepo(database).list(userIdOf(request), *limit, cursor);
            if (!page)
                co_return errorResponse(drogon::k400BadRequest, "invalid_cursor", "the cursor is not valid");

            json documents = json::array();
            for (const vec::Document& doc : page->documents) documents.push_back(toJson(doc));
            co_return jsonResponse(
                drogon::k200OK, {{"documents", documents},
                                 {"nextCursor", page->nextCursor ? json(*page->nextCursor) : json(nullptr)}});
        },
        drogon::Get);

    registerProtected(
        "/api/docs/{id}",
        [database](HttpRequestPtr request, std::string id) -> drogon::Task<HttpResponsePtr> {
            const auto role = co_await requireRole(database, id, userIdOf(request), vec::Role::Viewer);
            if (!role) co_return role.error();
            const auto doc = co_await vec::DocRepo(database).get(id);
            if (!doc) co_return notFound();  // deleted since the role check

            // The newest snapshot, then every op after it, up to OpRepo's limit: enough to rebuild the
            // drawing. If the limit cut the ops short, the rest come over the live connection.
            const auto snapshot = co_await vec::SnapshotRepo(database).latest(id);
            const int64_t since = snapshot ? snapshot->seq : 0;
            const auto ops = co_await vec::OpRepo(database).opsAfter(id, since);

            json opsJson = json::array();
            for (const vec::OpRow& op : ops) {
                opsJson.push_back({{"seq", op.seq},
                                   {"userId", op.userId},
                                   {"clientOpId", op.clientOpId},
                                   {"op", op.payload},
                                   {"createdAt", op.createdAt}});
            }
            co_return jsonResponse(
                drogon::k200OK,
                {{"document", toJson(*doc)},
                 {"role", vec::toString(*role)},
                 {"snapshot",
                  snapshot ? json{{"seq", snapshot->seq}, {"state", snapshot->state}} : json(nullptr)},
                 {"ops", opsJson},
                 {"opsComplete", static_cast<int>(ops.size()) < vec::OpRepo::maxOpsPerRead}});
        },
        drogon::Get);

    registerProtected(
        "/api/docs/{id}",
        [database](HttpRequestPtr request, std::string id) -> drogon::Task<HttpResponsePtr> {
            const auto role = co_await requireRole(database, id, userIdOf(request), vec::Role::Editor);
            if (!role) co_return role.error();
            const auto title = titleFrom(request);
            if (!title) co_return title.error();

            const auto doc = co_await vec::DocRepo(database).rename(id, *title);
            if (!doc) co_return notFound();
            co_return jsonResponse(drogon::k200OK, toJson(*doc));
        },
        drogon::Patch);

    registerProtected(
        "/api/docs/{id}",
        [database](HttpRequestPtr request, std::string id) -> drogon::Task<HttpResponsePtr> {
            const auto role = co_await requireRole(database, id, userIdOf(request), vec::Role::Owner);
            if (!role) co_return role.error();
            if (!co_await vec::DocRepo(database).remove(id)) co_return notFound();

            const auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k204NoContent);
            co_return response;
        },
        drogon::Delete);
}
