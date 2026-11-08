#include "ExportRoutes.h"

#include <drogon/drogon.h>

#include <filesystem>
#include <optional>
#include <string>

#include "ApiResponse.h"
#include "AuthFilter.h"
#include "Routes.h"
#include "db/DocRepo.h"
#include "db/ExportRepo.h"
#include "exports/RunExport.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using nlohmann::json;

namespace {

json toJson(const vec::ExportJob& job) {
    const auto orNull = [](const std::optional<std::string>& text) {
        return text ? json(*text) : json(nullptr);
    };
    return {{"id", job.id},
            {"documentId", job.documentId},
            {"format", job.format},
            {"status", job.status},
            {"attempts", job.attempts},
            {"resultUrl", orNull(job.resultUrl)},
            {"error", orNull(job.error)},
            {"createdAt", job.createdAt}};
}

HttpResponsePtr noSuchExport() { return errorResponse(drogon::k404NotFound, "not_found", "no such export"); }

}  // namespace

void registerExportRoutes(const drogon::orm::DbClientPtr& database, const std::string& exportDir) {
    registerProtected(
        "/api/docs/{id}/exports",
        [database](HttpRequestPtr request, std::string id) -> drogon::Task<HttpResponsePtr> {
            const auto role = co_await requireRole(database, id, userIdOf(request), vec::Role::Viewer);
            if (!role) co_return role.error();

            const json body = json::parse(request->body(), nullptr, false);
            const auto format = body.is_object() ? body.find("format") : body.end();
            if (format == body.end() || !format->is_string() ||
                !vec::isExportFormat(format->get<std::string>())) {
                co_return errorResponse(drogon::k400BadRequest, "invalid_format",
                                        "format must be svg or png");
            }

            const vec::ExportJob job =
                co_await vec::ExportRepo(database).enqueue(id, format->get<std::string>());
            auto response = jsonResponse(drogon::k202Accepted, toJson(job));
            response->addHeader("Location", "/api/exports/" + job.id);
            co_return response;
        },
        drogon::Post);

    registerProtected(
        "/api/exports/{id}",
        [database](HttpRequestPtr request, std::string id) -> drogon::Task<HttpResponsePtr> {
            const auto job = co_await vec::ExportRepo(database).get(id);
            // Someone who can't see the document can't tell its exports exist either.
            if (!job || !co_await vec::MemberRepo(database).roleOf(job->documentId, userIdOf(request))) {
                co_return noSuchExport();
            }
            co_return jsonResponse(drogon::k200OK, toJson(*job));
        },
        drogon::Get);

    registerProtected(
        "/api/exports/{id}/file",
        [database, exportDir](HttpRequestPtr request, std::string id) -> drogon::Task<HttpResponsePtr> {
            const auto job = co_await vec::ExportRepo(database).get(id);
            if (!job || !co_await vec::MemberRepo(database).roleOf(job->documentId, userIdOf(request))) {
                co_return noSuchExport();
            }
            if (job->status != "done") {
                co_return errorResponse(drogon::k409Conflict, "not_ready", "the export is " + job->status);
            }
            const std::filesystem::path path = vec::exportFilePath(exportDir, *job);
            std::error_code error;
            if (!std::filesystem::is_regular_file(path, error)) {
                co_return errorResponse(drogon::k410Gone, "gone", "the file is no longer available");
            }

            const bool svg = job->format == "svg";
            auto response =
                drogon::HttpResponse::newFileResponse(path.string(), svg ? "drawing.svg" : "drawing.png",
                                                      svg ? drogon::CT_IMAGE_SVG_XML : drogon::CT_IMAGE_PNG);
            // An SVG opened from this origin could run script; ours never contains any, but don't rely on it.
            response->addHeader("Content-Security-Policy", "default-src 'none'");
            response->addHeader("X-Content-Type-Options", "nosniff");
            response->addHeader("Cache-Control", "private, no-store");
            co_return response;
        },
        drogon::Get);
}
