#include <trantor/net/EventLoopThread.h>

#include <chrono>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "DatabaseTest.h"
#include "collab/LoadDrawing.h"
#include "collab/OpPipeline.h"
#include "collab/Snapshots.h"
#include "db/DocRepo.h"
#include "db/OpRepo.h"
#include "ops/Apply.h"
#include "ops/Json.h"

using namespace vec;
using nlohmann::json;

namespace {

// Collects what a session is sent.
struct Inbox {
    std::mutex mutex;
    std::vector<json> messages;

    std::function<void(const std::string&)> sink() {
        return [this](const std::string& text) {
            const std::lock_guard lock(mutex);
            messages.push_back(json::parse(text));
        };
    }
    json last() {
        const std::lock_guard lock(mutex);
        return messages.empty() ? json() : messages.back();
    }
    size_t size() {
        const std::lock_guard lock(mutex);
        return messages.size();
    }
};

// A loaded room for a fresh document holding one rectangle, "a", at x = 0.
class OpPipelineTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        loopThread.run();
        user = newUser();
        document = drogon::sync_wait(DocRepo(database).create(user.id, "doc"));
        Diagram drawing;
        ASSERT_TRUE(vec::apply(drawing, AddShape{"a", {RectGeom{0, 0, 10, 10}, {}}, std::nullopt}));
        ASSERT_TRUE(drogon::sync_wait(SnapshotRepo(database).insert(document.id, 0, diagramToJson(drawing))));

        room = std::make_shared<Room>(document.id, loopThread.getLoop());
        auto loaded = drogon::sync_wait(loadDrawing(database, document.id));
        ASSERT_TRUE(loaded);
        onLoop([&] {
            room->finishLoad(std::move(loaded->diagram), loaded->lastSeq);
            room->join(watcherSession, {"watcher", watcher.sink()});
            room->markJoined(watcherSession);
        });
    }

    void onLoop(const std::function<void()>& work) {
        std::promise<void> done;
        loopThread.getLoop()->queueInLoop([&] {
            work();
            done.set_value();
        });
        done.get_future().wait();
    }

    // Runs one op through the pipeline as `session`, and returns the answer it got.
    json send(const std::string& clientOpId, const Operation& op, SessionId session = senderSession) {
        drogon::sync_wait(processOp(database, room, {session, user.id, sender.sink()}, {clientOpId, 0, op},
                                    std::chrono::steady_clock::now()));
        return sender.last();
    }

    int64_t storedOps() {
        return database
            ->execSqlSync("SELECT count(*) FROM operations WHERE document_id = $1::uuid", document.id)[0][0]
            .as<int64_t>();
    }

    double xOfA() {
        double x = 0;
        onLoop([&] { x = room->diagram().find("a")->getX(); });
        return x;
    }

    static constexpr SessionId senderSession = 1;
    static constexpr SessionId watcherSession = 2;

    trantor::EventLoopThread loopThread;
    User user;
    Document document;
    std::shared_ptr<Room> room;
    Inbox sender, watcher;
};

const Operation moveA = Move{"a", 5, 0};

}  // namespace

TEST_F(OpPipelineTest, AnOpIsStoredAppliedAckedAndBroadcast) {
    const json answer = send("c1", moveA);

    EXPECT_EQ(answer, json({{"type", "ack"}, {"clientOpId", "c1"}, {"seq", 1}}));
    EXPECT_EQ(storedOps(), 1);
    EXPECT_DOUBLE_EQ(xOfA(), 5);
    EXPECT_EQ(watcher.last().at("type"), "op");
    EXPECT_EQ(watcher.last().at("seq"), 1);
}

TEST_F(OpPipelineTest, ADuplicateClientOpIdGetsTheOriginalAckAndChangesNothing) {
    ASSERT_EQ(send("c1", moveA).at("seq"), 1);
    const size_t broadcasts = watcher.size();

    const json answer = send("c1", moveA);

    EXPECT_EQ(answer, json({{"type", "ack"}, {"clientOpId", "c1"}, {"seq", 1}}));
    EXPECT_EQ(storedOps(), 1);
    EXPECT_DOUBLE_EQ(xOfA(), 5);            // moved once, not twice
    EXPECT_EQ(watcher.size(), broadcasts);  // nobody is told about it again
}

TEST_F(OpPipelineTest, ADuplicateIsRecognisedAfterOtherOps) {
    send("c1", moveA);
    send("c2", moveA);

    EXPECT_EQ(send("c1", moveA).at("seq"), 1);
    EXPECT_EQ(storedOps(), 2);
}

TEST_F(OpPipelineTest, ARetriedAddShapeIsAckedNotRejectedAsADuplicateShape) {
    const Operation add = AddShape{"b", {CircleGeom{{1, 1}, 2}, {}}, std::nullopt};
    ASSERT_EQ(send("add-b", add).at("type"), "ack");

    const json retry = send("add-b", add);

    EXPECT_EQ(retry.at("type"), "ack");
    EXPECT_EQ(retry.at("seq"), 1);
}

TEST_F(OpPipelineTest, ARetryFromANewConnectionIsStillRecognised) {
    send("c1", moveA, senderSession);

    EXPECT_EQ(send("c1", moveA, 99).at("seq"), 1);
    EXPECT_DOUBLE_EQ(xOfA(), 5);
}

TEST_F(OpPipelineTest, AnOpThatDoesNotApplyIsRejectedAndNotStored) {
    const json answer = send("c1", Move{"missing", 1, 1});

    EXPECT_EQ(answer.at("type"), "reject");
    EXPECT_EQ(answer.at("reason"), "shape_not_found");
    EXPECT_EQ(storedOps(), 0);
    EXPECT_EQ(watcher.size(), 0u);
}

TEST_F(OpPipelineTest, ARejectedOpCanBeRetriedOnceItApplies) {
    const Operation add = AddShape{"b", {CircleGeom{{1, 1}, 2}, {}}, std::nullopt};
    ASSERT_EQ(send("move-b", Move{"b", 1, 0}).at("type"), "reject");
    send("add-b", add);

    EXPECT_EQ(send("move-b", Move{"b", 1, 0}).at("type"), "ack");
}

TEST_F(OpPipelineTest, OpsFromAnotherInstanceAreCaughtUpAndBroadcastFirst) {
    database->execSqlSync(
        "INSERT INTO operations (document_id, seq, user_id, client_op_id, payload) "
        "VALUES ($1::uuid, 1, $2::uuid, 'elsewhere', '{\"kind\":\"move\",\"id\":\"a\",\"dx\":100,\"dy\":0}')",
        document.id, user.id);
    database->execSqlSync("UPDATE documents SET head_seq = 1 WHERE id = $1::uuid", document.id);

    EXPECT_EQ(send("c1", moveA).at("seq"), 2);
    EXPECT_DOUBLE_EQ(xOfA(), 105);
    ASSERT_EQ(watcher.size(), 2u);
    EXPECT_EQ(watcher.messages[0].at("seq"), 1);
    EXPECT_EQ(watcher.messages[1].at("seq"), 2);
}

namespace {

// Waits for the background snapshot write, failing after a few seconds.
std::optional<SnapshotRow> waitForSnapshot(const drogon::orm::DbClientPtr& database,
                                           const std::string& documentId, int64_t seq) {
    for (int i = 0; i < 100; ++i) {
        auto latest = drogon::sync_wait(SnapshotRepo(database).latest(documentId));
        if (latest && latest->seq >= seq) return latest;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    return std::nullopt;
}

}  // namespace

TEST_F(OpPipelineTest, ASnapshotIsSavedEverySnapshotIntervalOps) {
    for (int64_t i = 1; i < snapshotInterval; ++i) send("m" + std::to_string(i), moveA);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(drogon::sync_wait(SnapshotRepo(database).latest(document.id))->seq, 0);  // only the original

    send("m-last", moveA);

    const auto snapshot = waitForSnapshot(database, document.id, snapshotInterval);
    ASSERT_TRUE(snapshot);
    EXPECT_EQ(snapshot->seq, snapshotInterval);
    // The snapshot holds the drawing as of that op: "a" moved by 5, snapshotInterval times.
    const auto drawing = decodeDiagram(snapshot->state);
    ASSERT_TRUE(drawing);
    EXPECT_DOUBLE_EQ(drawing->find("a")->getX(), 5.0 * snapshotInterval);
}

TEST_F(OpPipelineTest, LoadingFromTheNewSnapshotGivesTheSameDrawing) {
    for (int64_t i = 1; i <= snapshotInterval + 7; ++i) send("m" + std::to_string(i), moveA);
    ASSERT_TRUE(waitForSnapshot(database, document.id, snapshotInterval));

    const auto loaded = drogon::sync_wait(loadDrawing(database, document.id));

    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->snapshotSeq, snapshotInterval);
    EXPECT_EQ(loaded->lastSeq, snapshotInterval + 7);
    EXPECT_DOUBLE_EQ(loaded->diagram.find("a")->getX(), xOfA());
}
