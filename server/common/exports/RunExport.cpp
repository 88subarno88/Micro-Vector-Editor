#include "exports/RunExport.h"

#include <fstream>
#include <system_error>
#include <tl/expected.hpp>

#include "collab/LoadDrawing.h"
#include "db/ExportRepo.h"
#include "logging/Log.h"
#include "render/Png.h"

namespace vec {

namespace {

tl::expected<std::string, std::string> render(const Diagram& diagram, const std::string& format) {
    if (format == "svg") return diagram.toSVG() + '\n';
    auto png = renderPng(diagram);
    if (!png) return tl::unexpected(png.error());
    return std::move(png->bytes);
}

// Writes a temporary file and renames it over `path`, so a download never sees half a file. False,
// leaving nothing behind, if either step fails.
bool writeWhole(const std::filesystem::path& path, const std::string& contents) {
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    std::ofstream out(temporary, std::ios::binary);
    out << contents;
    out.close();
    std::error_code error;
    if (out) std::filesystem::rename(temporary, path, error);
    if (!out || error) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

}  // namespace

std::string exportResultUrl(const std::string& jobId) { return "/api/exports/" + jobId + "/file"; }

std::filesystem::path exportFilePath(const std::filesystem::path& directory, const ExportJob& job) {
    return directory / (job.id + "." + job.format);
}

drogon::Task<> runExport(drogon::orm::DbClientPtr database, ExportJob job, std::filesystem::path directory) {
    ExportRepo jobs(database);
    const auto failWith = [&](const std::string& reason, const std::string& detail) -> drogon::Task<> {
        logError(
            "export failed", {},
            {{"job", job.id}, {"document", job.documentId}, {"attempt", job.attempts}, {"error", detail}});
        co_await jobs.fail(job.id, reason);
    };

    const auto drawing = co_await loadDrawing(database, job.documentId);
    if (!drawing) co_return co_await failWith("the drawing couldn't be loaded", drawing.error());
    const auto contents = render(drawing->diagram, job.format);
    if (!contents) co_return co_await failWith("the drawing couldn't be rendered", contents.error());

    const std::filesystem::path path = exportFilePath(directory, job);
    if (!writeWhole(path, *contents)) {
        co_return co_await failWith("the file couldn't be saved", "cannot write " + path.string());
    }
    if (!co_await jobs.finish(job.id, exportResultUrl(job.id))) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);  // the job is gone, so nothing will ever serve the file
        co_return;
    }
    logInfo("exported", {},
            {{"job", job.id},
             {"document", job.documentId},
             {"format", job.format},
             {"bytes", contents->size()},
             {"lastSeq", drawing->lastSeq}});
}

}  // namespace vec
