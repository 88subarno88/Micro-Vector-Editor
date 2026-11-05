#ifndef SERVER_RUN_EXPORT_H
#define SERVER_RUN_EXPORT_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <filesystem>
#include <string>

#include "db/Models.h"

namespace vec {

// Where a finished export can be downloaded.
std::string exportResultUrl(const std::string& jobId);

// Where a job's file is saved in `directory`: <job id>.<format>.
std::filesystem::path exportFilePath(const std::filesystem::path& directory, const ExportJob& job);

// Does a claimed (running) job: renders the document's current drawing in the job's format, saves it to
// exportFilePath(), and marks the job done, or failed with a reason fit to show its user. Details of a
// failure are logged. Database failures throw DrogonDbException, leaving the job running.
drogon::Task<> runExport(drogon::orm::DbClientPtr database, ExportJob job, std::filesystem::path directory);

}  // namespace vec

#endif  // SERVER_RUN_EXPORT_H
