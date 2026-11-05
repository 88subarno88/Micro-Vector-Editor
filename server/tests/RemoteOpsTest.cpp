#include <trantor/net/EventLoopThread.h>

#include <future>
#include <mutex>
#include <string>
#include <vector>

#include "DatabaseTest.h"
#include "collab/RemoteOps.h"
#include "db/DocRepo.h"
#include "ops/Apply.h"
#include "ops/Json.h"

using namespace vec;
using nlohmann::json;

namespace {

// A loaded room for a document with one rectangle, "a", and a joined member who records what it receives.
class RemoteOpsTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        loopThread.run();
        user = newUser();
        document = drogon::sync_wait(DocRepo(database).create(user.id, "doc"));
        room = std::make_shared<Room>(document.id, loopThread.getLoop());
        onLoop([&] {
            Diagram drawing;
            ASSERT_TRUE(vec::apply(drawing, AddShape{"a", {RectGeom{0, 0, 10, 10}, {}}, std::nullopt}));
            room->finishLoad(std::move(drawing), 0);
            room->join(1, {"watcher", [this](const std::string& m) {
                               const std::lock_guard lock(mutex);
                               received.push_back(json::parse(m).at("seq").get<int64_t>());
                           }});
            room->markJoined(1);
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

    // Stores a move of "a" by `dx` at `seq`, as another instance would.
    void storeMove(int64_t seq, double dx) {
        database->execSqlSync(
            "INSERT INTO operations (document_id, seq, user_id, client_op_id, payload) "
            "VALUES ($1::uuid, $2::bigint, $3::uuid, $4, $5::jsonb)",
            document.id, seq, user.id, "op-" + std::to_string(seq), json(Operation{Move{"a", dx, 0}}).dump());
    }

    std::string published(int64_t seq, double dx, const std::string& instance = "other") {
        return json{
            {"seq", seq}, {"userId", user.id}, {"op", Operation{Move{"a", dx, 0}}}, {"instance", instance}}
            .dump();
    }

    void deliver(const std::string& message) {
        drogon::sync_wait(applyRemoteOp(database, room, message, "me"));
    }

    int64_t lastSeq() {
        int64_t seq = 0;
        onLoop([&] { seq = room->lastSeq(); });
        return seq;
    }
    double xOfA() {
        double x = 0;
        onLoop([&] { x = room->diagram().find("a")->getX(); });
        return x;
    }

    trantor::EventLoopThread loopThread;
    User user;
    Document document;
    std::shared_ptr<Room> room;
    std::mutex mutex;
    std::vector<int64_t> received;
};

}  // namespace

TEST_F(RemoteOpsTest, TheNextOpIsAppliedAndBroadcast) {
    storeMove(1, 5);

    deliver(published(1, 5));

    EXPECT_EQ(lastSeq(), 1);
    EXPECT_DOUBLE_EQ(xOfA(), 5);
    EXPECT_EQ(received, std::vector<int64_t>{1});
}

TEST_F(RemoteOpsTest, OurOwnMessagesAreIgnored) {
    storeMove(1, 5);

    deliver(published(1, 5, "me"));

    EXPECT_EQ(lastSeq(), 0);
    EXPECT_TRUE(received.empty());
}

TEST_F(RemoteOpsTest, AnOpTheRoomAlreadyHasIsIgnored) {
    storeMove(1, 5);
    deliver(published(1, 5));

    deliver(published(1, 5));

    EXPECT_DOUBLE_EQ(xOfA(), 5);  // not moved twice
    EXPECT_EQ(received, std::vector<int64_t>{1});
}

TEST_F(RemoteOpsTest, AGapIsFilledFromTheDatabaseInOrder) {
    storeMove(1, 1);
    storeMove(2, 10);
    storeMove(3, 100);

    deliver(published(3, 100));  // 1 and 2 never arrived

    EXPECT_EQ(lastSeq(), 3);
    EXPECT_DOUBLE_EQ(xOfA(), 111);
    EXPECT_EQ(received, (std::vector<int64_t>{1, 2, 3}));
}

TEST_F(RemoteOpsTest, MessagesArrivingOutOfOrderEndUpApplyingEachOpOnce) {
    storeMove(1, 1);
    storeMove(2, 10);

    deliver(published(2, 10));  // fills 1 from the database
    deliver(published(1, 1));   // late: already applied

    EXPECT_DOUBLE_EQ(xOfA(), 11);
    EXPECT_EQ(received, (std::vector<int64_t>{1, 2}));
}

// Messages come only from this server's own instances, which publish exactly what they committed, so the
// next op is taken from the message rather than read back from Postgres. This documents that trust: a
// message that disagreed with the log would be believed.
TEST_F(RemoteOpsTest, TheNextOpIsTakenFromTheMessageWithoutReadingTheLog) {
    storeMove(1, 5);

    deliver(published(1, 999));

    EXPECT_DOUBLE_EQ(xOfA(), 999);
    EXPECT_EQ(lastSeq(), 1);
}

TEST_F(RemoteOpsTest, MalformedMessagesAreIgnored) {
    for (const char* message : {"", "not json", "[]", R"({"seq":"1"})", R"({"op":{}})", R"({"seq":1.5})"}) {
        deliver(message);
    }

    EXPECT_EQ(lastSeq(), 0);
    EXPECT_TRUE(received.empty());
}
