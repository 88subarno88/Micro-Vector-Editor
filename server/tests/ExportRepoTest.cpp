#include <optional>
#include <string>

#include "DatabaseTest.h"
#include "db/DocRepo.h"
#include "db/ExportRepo.h"

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
