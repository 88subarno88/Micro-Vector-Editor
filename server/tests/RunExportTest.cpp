#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "DatabaseTest.h"
#include "db/DocRepo.h"
#include "db/ExportRepo.h"
#include "exports/RunExport.h"

using namespace vec;

namespace {

// A document with one rectangle, and a running export job for it, as a worker would have claimed it.
class RunExportTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        user = newUser();
        document = drogon::sync_wait(DocRepo(database).create(user.id, "doc"));
        storeOp(1,
                R"({"kind":"addShape","id":"a","shape":{"kind":"rect","x":10,"y":20,"width":30,"height":40,)"
                R"("cornerRadius":0,"style":{"stroke":"red","fill":"#00ff00","strokeWidth":2}}})");
        directory = std::filesystem::temp_directory_path() / ("vec-export-test-" + marker);
        std::filesystem::create_directories(directory);
    }

    void TearDown() override {
        std::filesystem::remove_all(directory);
        DatabaseTest::TearDown();
    }

    void storeOp(int64_t seq, const std::string& payload) {
        database->execSqlSync(
            "INSERT INTO operations (document_id, seq, user_id, client_op_id, payload) "
            "VALUES ($1::uuid, $2::bigint, $3::uuid, $4, $5::jsonb)",
            document.id, seq, user.id, "op-" + std::to_string(seq), payload);
    }

    ExportJob runningJob(const std::string& format) {
        ExportJob job = drogon::sync_wait(ExportRepo(database).enqueue(document.id, format));
        database->execSqlSync("UPDATE export_jobs SET status = 'running' WHERE id = $1::uuid", job.id);
        return job;
    }

    ExportJob run(const ExportJob& job) {
        drogon::sync_wait(runExport(database, job, directory));
        return *drogon::sync_wait(ExportRepo(database).get(job.id));
    }

    static std::string read(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }

    User user;
    Document document;
    std::filesystem::path directory;
};

}  // namespace

TEST_F(RunExportTest, AnSvgExportSavesTheDrawingAndFinishesTheJob) {
    const ExportJob job = runningJob("svg");

    const ExportJob after = run(job);

    EXPECT_EQ(after.status, "done");
    EXPECT_EQ(after.resultUrl, "/api/exports/" + job.id + "/file");
    const std::string svg = read(exportFilePath(directory, job));
    EXPECT_EQ(svg.rfind("<svg", 0), 0u) << svg;
    EXPECT_NE(svg.find(R"(id="a")"), std::string::npos) << svg;
}

TEST_F(RunExportTest, APngExportSavesAnImage) {
    const ExportJob job = runningJob("png");

    EXPECT_EQ(run(job).status, "done");

    const std::string png = read(exportFilePath(directory, job));
    EXPECT_EQ(png.substr(0, 8), "\x89PNG\r\n\x1a\n");
}

TEST_F(RunExportTest, TheLatestEditsAreExported) {
    storeOp(2, R"({"kind":"deleteShape","id":"a"})");
    const ExportJob job = runningJob("svg");

    run(job);

    EXPECT_EQ(read(exportFilePath(directory, job)).find(R"(id="a")"), std::string::npos);
}

TEST_F(RunExportTest, ADrawingThatCantBeLoadedFailsTheJob) {
    storeOp(2, R"({"kind":"move","id":"no-such-shape","dx":1,"dy":0})");
    const ExportJob job = runningJob("svg");

    const ExportJob after = run(job);

    EXPECT_EQ(after.status, "failed");
    EXPECT_EQ(after.error, "the drawing couldn't be loaded");
    EXPECT_FALSE(std::filesystem::exists(exportFilePath(directory, job)));
}

TEST_F(RunExportTest, AFileThatCantBeSavedFailsTheJobAndLeavesNothingBehind) {
    const ExportJob job = runningJob("svg");
    std::filesystem::remove_all(directory);  // gone, so nothing can be written there

    const ExportJob after = run(job);

    EXPECT_EQ(after.status, "failed");
    EXPECT_EQ(after.error, "the file couldn't be saved");
    EXPECT_EQ(after.resultUrl, std::nullopt);
}

TEST_F(RunExportTest, AJobThatIsNoLongerRunningKeepsItsStatusAndLeavesNoFile) {
    const ExportJob job = runningJob("svg");
    database->execSqlSync("UPDATE export_jobs SET status = 'failed' WHERE id = $1::uuid", job.id);

    EXPECT_EQ(run(job).status, "failed");
    EXPECT_FALSE(std::filesystem::exists(exportFilePath(directory, job)));
}
