#ifndef SERVER_EXPORT_REPO_H
#define SERVER_EXPORT_REPO_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "db/Models.h"

namespace vec {

// Whether `format` is one a document can be exported as: "svg" or "png".
bool isExportFormat(std::string_view format);

// Reads and writes the export_jobs table, which is also the workers' queue. An `id` that isn't a UUID
// finds nothing. Unexpected database failures throw DrogonDbException.
class ExportRepo {
public:
    explicit ExportRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // Queues an export of the document in `format`. If one in that format is already queued, returns it
    // instead: it hasn't started, so it will include every edit made until it does. `format` must pass
    // isExportFormat().
    drogon::Task<ExportJob> enqueue(const std::string& documentId, const std::string& format);
    drogon::Task<std::optional<ExportJob>> get(const std::string& id);

    // Takes the oldest queued job whose run_after has passed, marks it running and counts the attempt;
    // nullopt if none is due. Rows another worker is claiming at the same moment are skipped, not waited
    // for, so each job goes to exactly one worker.
    drogon::Task<std::optional<ExportJob>> claim();

    // Mark a running job done, with where to download the file, or failed, with why. False, changing
    // nothing, if the job isn't running, such as when its document was deleted meanwhile.
    drogon::Task<bool> finish(const std::string& id, const std::string& resultUrl);
    drogon::Task<bool> fail(const std::string& id, const std::string& error);

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_EXPORT_REPO_H
