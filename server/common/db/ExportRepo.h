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

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_EXPORT_REPO_H
