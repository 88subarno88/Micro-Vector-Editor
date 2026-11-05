#ifndef SERVER_EXPORT_REPO_H
#define SERVER_EXPORT_REPO_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
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
//
// A job is tried up to maxAttempts times. run_after is when it may next be claimed: for a queued job,
// after any retry delay; for a running one, when its worker's lease ends. A running job whose lease has
// ended was abandoned, by a worker that crashed say, and is claimed again like a queued one.
class ExportRepo {
public:
    static constexpr int maxAttempts = 5;
    // Doubles after each failed attempt: 5, 10, 20, then 40 s.
    static constexpr std::chrono::seconds firstRetryDelay{5};
    // Far longer than an export takes, so a working worker never loses its job.
    static constexpr std::chrono::seconds lease{300};

    explicit ExportRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // Queues an export of the document in `format`. If one in that format is already queued, returns it
    // instead: it hasn't started, so it will include every edit made until it does. `format` must pass
    // isExportFormat().
    drogon::Task<ExportJob> enqueue(const std::string& documentId, const std::string& format);
    drogon::Task<std::optional<ExportJob>> get(const std::string& id);

    // Takes the oldest due job, queued or abandoned, with attempts left; marks it running, counts the
    // attempt and starts its lease. nullopt if none is due. Rows another worker is claiming at the same
    // moment are skipped, not waited for, so each job goes to one worker at a time.
    drogon::Task<std::optional<ExportJob>> claim();

    // Marks a running job done, with where to download the file. False, changing nothing, if the job
    // isn't running, such as when its document was deleted meanwhile.
    drogon::Task<bool> finish(const std::string& id, const std::string& resultUrl);
    // Records why a running job's attempt failed, and queues it to be tried again after the retry delay,
    // or marks it failed if that was its last attempt. False, changing nothing, if it isn't running.
    drogon::Task<bool> fail(const std::string& id, const std::string& error);
    // Marks failed every abandoned job that has no attempts left, which claim() would never take again.
    // Returns how many.
    drogon::Task<size_t> failAbandoned();

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_EXPORT_REPO_H
