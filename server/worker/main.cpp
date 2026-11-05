// The export worker: takes queued export jobs from Postgres one at a time. Run as many as needed; each
// job goes to exactly one of them. Needs DATABASE_URL, and EXPORT_DIR for the files, a folder the API
// can read too; it's created if missing. Stops after the current job on SIGINT or SIGTERM.
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <drogon/utils/coroutine.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <optional>
#include <system_error>
#include <thread>

#include "config/Config.h"
#include "db/ExportRepo.h"
#include "exports/RunExport.h"
#include "logging/Log.h"

namespace {

// How long to wait before looking again when no job is due.
constexpr std::chrono::seconds pollInterval{1};
// Well inside the 10 s a container gets to stop, since a claim in progress delays shutting down.
constexpr double databaseTimeoutSeconds = 5;

std::atomic<bool> stopping{false};
static_assert(std::atomic<bool>::is_always_lock_free, "set from a signal handler");

void stop(int) { stopping = true; }

}  // namespace

int main() {
    vec::initLogging();

    const auto databaseUrl = vec::readEnv("DATABASE_URL");
    if (!databaseUrl || databaseUrl->empty()) {
        std::cerr << "DATABASE_URL is not set\n";
        return 1;
    }
    const auto exportDir = vec::readEnv("EXPORT_DIR");
    if (!exportDir || exportDir->empty()) {
        std::cerr << "EXPORT_DIR is not set\n";
        return 1;
    }
    std::error_code error;
    std::filesystem::create_directories(*exportDir, error);
    if (error) {
        std::cerr << "cannot create " << *exportDir << ": " << error.message() << '\n';
        return 1;
    }
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);

    const auto database = drogon::orm::DbClient::newPgClient(*databaseUrl, 1);
    // Otherwise a claim waits forever while the database is down, and so does shutting down.
    database->setTimeout(databaseTimeoutSeconds);
    vec::ExportRepo jobs(database);
    vec::logInfo("worker started");

    while (!stopping) {
        std::optional<vec::ExportJob> job;
        try {
            job = drogon::sync_wait(jobs.claim());
            if (!job) {
                if (const size_t abandoned = drogon::sync_wait(jobs.failAbandoned())) {
                    vec::logWarning("abandoned exports failed", {}, {{"jobs", abandoned}});
                }
            }
        } catch (const drogon::orm::DrogonDbException& e) {
            vec::logError("claim failed", {}, {{"error", e.base().what()}});
        }
        if (!job) {
            std::this_thread::sleep_for(pollInterval);
            continue;
        }
        try {
            drogon::sync_wait(vec::runExport(database, *job, *exportDir));
        } catch (const drogon::orm::DrogonDbException& e) {
            vec::logError("export interrupted", {}, {{"job", job->id}, {"error", e.base().what()}});
        }
    }
    vec::logInfo("worker stopped");
}
