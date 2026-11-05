#include <optional>
#include <string>
#include <utility>

#include "DatabaseTest.h"
#include "db/DocRepo.h"
#include "db/ExportRepo.h"
#include "db/OpRepo.h"

using namespace vec;

namespace {

class ExportRepoTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        document = drogon::sync_wait(DocRepo(database).create(newUser().id, "doc"));
    }

    ExportJob enqueue(const std::string& format) {
        return drogon::sync_wait(ExportRepo(database).enqueue(document.id, format));
    }

    Document document;
};

}  // namespace

TEST(ExportFormatTest, OnlySvgAndPngAreFormats) {
    EXPECT_TRUE(isExportFormat("svg"));
    EXPECT_TRUE(isExportFormat("png"));
    for (const char* format : {"", "SVG", "pdf", "svg "}) EXPECT_FALSE(isExportFormat(format)) << format;
}

TEST_F(ExportRepoTest, EnqueueCreatesAQueuedJob) {
    const ExportJob job = enqueue("png");

    EXPECT_TRUE(isUuid(job.id));
    EXPECT_EQ(job.documentId, document.id);
    EXPECT_EQ(job.format, "png");
    EXPECT_EQ(job.status, "queued");
    EXPECT_EQ(job.attempts, 0);
    EXPECT_EQ(job.resultUrl, std::nullopt);
    EXPECT_EQ(job.error, std::nullopt);
    EXPECT_FALSE(job.createdAt.empty());
}

TEST_F(ExportRepoTest, GetFindsAJobButNotAnUnknownOne) {
    const ExportJob job = enqueue("svg");
    ExportRepo repo(database);

    const auto found = drogon::sync_wait(repo.get(job.id));
    ASSERT_TRUE(found);
    EXPECT_EQ(found->id, job.id);
    EXPECT_EQ(found->status, "queued");

    EXPECT_EQ(drogon::sync_wait(repo.get("00000000-0000-0000-0000-000000000000")), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(repo.get("not-a-uuid")), std::nullopt);
}

TEST_F(ExportRepoTest, AQueuedJobInTheSameFormatIsReused) {
    const ExportJob first = enqueue("png");

    EXPECT_EQ(enqueue("png").id, first.id);
    EXPECT_NE(enqueue("svg").id, first.id);
}

TEST_F(ExportRepoTest, AJobThatHasStartedIsNotReused) {
    const ExportJob first = enqueue("png");
    for (const char* status : {"running", "done", "failed"}) {
        database->execSqlSync("UPDATE export_jobs SET status = $1 WHERE id = $2::uuid", std::string(status),
                              first.id);

        EXPECT_NE(enqueue("png").id, first.id) << status;
        database->execSqlSync("DELETE FROM export_jobs WHERE id <> $1::uuid AND document_id = $2::uuid",
                              first.id, document.id);
    }
}

TEST_F(ExportRepoTest, JobsAreDeletedWithTheirDocument) {
    const ExportJob job = enqueue("svg");

    drogon::sync_wait(DocRepo(database).remove(document.id));

    EXPECT_EQ(drogon::sync_wait(ExportRepo(database).get(job.id)), std::nullopt);
}

namespace {

// The queue is shared by every document, so these tests empty it first: run them against a database
// whose export jobs don't matter.
class ExportClaimTest : public ExportRepoTest {
protected:
    void SetUp() override {
        ExportRepoTest::SetUp();
        if (IsSkipped()) return;
        database->execSqlSync("DELETE FROM export_jobs");
    }

    std::optional<ExportJob> claim() { return drogon::sync_wait(ExportRepo(database).claim()); }
    // The claimed job's ID, or "nothing".
    std::string claimedId() {
        const auto job = claim();
        return job ? job->id : "nothing";
    }

    // A second connection, for holding a lock while the first claims. Never destroyed, like the first.
    drogon::orm::DbClientPtr otherConnection() {
        static const auto* const client = new drogon::orm::DbClientPtr(
            drogon::orm::DbClient::newPgClient(std::getenv("TEST_DATABASE_URL"), 1));
        return *client;
    }
};

}  // namespace

TEST_F(ExportClaimTest, NothingIsClaimedFromAnEmptyQueue) { EXPECT_EQ(claim(), std::nullopt); }

TEST_F(ExportClaimTest, TheOldestQueuedJobIsClaimedAndMarkedRunning) {
    const ExportJob older = enqueue("svg");
    const ExportJob newer = enqueue("png");

    const auto first = claim();
    ASSERT_TRUE(first);
    EXPECT_EQ(first->id, older.id);
    EXPECT_EQ(first->status, "running");
    EXPECT_EQ(first->attempts, 1);
    EXPECT_EQ(drogon::sync_wait(ExportRepo(database).get(older.id))->status, "running");

    EXPECT_EQ(claimedId(), newer.id);
    EXPECT_EQ(claim(), std::nullopt);  // a running job isn't claimed again
}

TEST_F(ExportClaimTest, AJobIsNotClaimedBeforeItsRunAfter) {
    const ExportJob job = enqueue("svg");
    database->execSqlSync("UPDATE export_jobs SET run_after = now() + interval '1 hour' WHERE id = $1::uuid",
                          job.id);

    EXPECT_EQ(claim(), std::nullopt);

    database->execSqlSync("UPDATE export_jobs SET run_after = now() WHERE id = $1::uuid", job.id);
    EXPECT_EQ(claimedId(), job.id);
}

TEST_F(ExportClaimTest, OnlyQueuedJobsAreClaimed) {
    const ExportJob job = enqueue("svg");
    for (const char* status : {"done", "failed"}) {
        database->execSqlSync("UPDATE export_jobs SET status = $1 WHERE id = $2::uuid", std::string(status),
                              job.id);
        EXPECT_EQ(claim(), std::nullopt) << status;
    }
}

TEST_F(ExportClaimTest, AJobAnotherWorkerIsClaimingIsSkippedNotWaitedFor) {
    const ExportJob locked = enqueue("svg");
    const ExportJob free = enqueue("png");
    // Another worker is midway through claiming `locked`.
    auto other = otherConnection()->newTransaction();
    other->execSqlSync("SELECT id FROM export_jobs WHERE id = $1::uuid FOR UPDATE", locked.id);

    EXPECT_EQ(claimedId(), free.id);
    EXPECT_EQ(claim(), std::nullopt);

    ASSERT_TRUE(drogon::sync_wait(commit(std::move(other))));  // waits until the lock is released
    EXPECT_EQ(claimedId(), locked.id);
}

TEST_F(ExportRepoTest, FinishAndFailChangeOnlyARunningJob) {
    ExportRepo repo(database);
    const ExportJob queued = enqueue("svg");
    EXPECT_FALSE(drogon::sync_wait(repo.finish(queued.id, "/file")));
    EXPECT_FALSE(drogon::sync_wait(repo.fail(queued.id, "no")));
    EXPECT_EQ(drogon::sync_wait(repo.get(queued.id))->status, "queued");

    database->execSqlSync("UPDATE export_jobs SET status = 'running' WHERE id = $1::uuid", queued.id);
    EXPECT_TRUE(drogon::sync_wait(repo.finish(queued.id, "/file")));
    const auto done = drogon::sync_wait(repo.get(queued.id));
    EXPECT_EQ(done->status, "done");
    EXPECT_EQ(done->resultUrl, "/file");
    EXPECT_FALSE(drogon::sync_wait(repo.fail(queued.id, "too late")));  // done stays done

    const ExportJob other = enqueue("png");
    database->execSqlSync("UPDATE export_jobs SET status = 'running' WHERE id = $1::uuid", other.id);
    EXPECT_TRUE(drogon::sync_wait(repo.fail(other.id, "broken")));
    const auto failed = drogon::sync_wait(repo.get(other.id));
    EXPECT_EQ(failed->status, "failed");
    EXPECT_EQ(failed->error, "broken");
    EXPECT_EQ(failed->resultUrl, std::nullopt);
}
