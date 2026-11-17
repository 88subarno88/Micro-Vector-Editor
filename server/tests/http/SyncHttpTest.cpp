#include <drogon/HttpRequest.h>
#include <drogon/WebSocketClient.h>

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include "LiveDocTest.h"
#include "auth/AccessToken.h"
#include "config/Config.h"
#include "db/OpRepo.h"

using namespace vec;
using nlohmann::json;

namespace {

// A document owned by alice and shared with bob as an editor, each with a live connection to it.
class SyncHttpTest : public LiveDocTest {
protected:
    void SetUp() override {
        LiveDocTest::SetUp();
        if (IsSkipped()) return;
        createSharedDocument("alice@example.com", {"bob@example.com"});
        alice = connect(ownerToken);
        bob = connect(editorTokens[0]);
    }

    static AddShape rect(const std::string& id) { return {id, {RectGeom{0, 0, 10, 10}, {}}, std::nullopt}; }
    static AddShape circle(const std::string& id) {
        return {id, {CircleGeom{{50, 50}, 5}, {}}, std::nullopt};
    }

    std::unique_ptr<DocClient> alice, bob;
};

}  // namespace

TEST_F(SyncHttpTest, AnEditByOneClientReachesTheOther) {
    alice->send(rect("a"));
    alice->send(Move{"a", 5, 0});

    expectEveryoneAgrees({alice.get(), bob.get()});
    EXPECT_EQ(bob->replica()["shapes"].size(), 1u);
    EXPECT_EQ(alice->acked(), 2);
}

TEST_F(SyncHttpTest, ConflictingEditsAtOnceLeaveBothClientsWithTheServersDrawing) {
    alice->send(rect("a"));
    alice->send(rect("b"));
    ASSERT_TRUE(alice->waitUntilSettled(2));

    // Sent at the same moment over two connections, so the server interleaves them however it likes.
    for (int i = 0; i < 15; ++i) {
        alice->send(Move{"a", 1, 0});
        bob->send(Resize{"a", 1.1});
        bob->send(Move{"b", 0, 1});
        if (i == 5) alice->send(DeleteShape{"b"});  // bob's later moves of b now fail
        if (i == 10) {
            alice->send(rect("c"));  // both add "c": only one can win
            bob->send(circle("c"));
        }
    }
    bob->send(SetStyle{"a", std::string("red"), std::nullopt, 3.0});
    alice->send(SetStyle{"a", std::nullopt, std::string("blue"), std::nullopt});

    expectEveryoneAgrees({alice.get(), bob.get()});
    EXPECT_EQ(alice->acked() + alice->rejected(), 2 + 15 + 1 + 1 + 1);
    EXPECT_EQ(bob->acked() + bob->rejected(), 30 + 1 + 1);
    EXPECT_GE(alice->rejected() + bob->rejected(), 1);    // the second "c"
    EXPECT_EQ(headSeq(), alice->acked() + bob->acked());  // every acknowledged op, and nothing else
    const json shapes = alice->replica()["shapes"];
    ASSERT_EQ(shapes.size(), 2u);  // a and c; b was deleted
}

TEST_F(SyncHttpTest, AClientThatJoinsLaterGetsTheSameDrawing) {
    alice->send(rect("a"));
    bob->send(rect("b"));
    alice->send(Move{"a", 3, 4});
    bob->send(DeleteShape{"a"});
    expectEveryoneAgrees({alice.get(), bob.get()});

    const auto late = connect(editorTokens[0]);

    expectEveryoneAgrees({alice.get(), bob.get(), late.get()});
}

// The first connection to a document closes while it's still joining, as when a page opens the document
// and is closed or reloaded at once. The next client must still be able to open it.
TEST_F(LiveDocTest, AConnectionThatClosesWhileJoiningLeavesTheDocumentOpenToOthers) {
    createSharedDocument("alice@example.com", {});
    for (int i = 0; i < 3;
         ++i) {  // the close must beat the join, which waits on the database: it nearly always does
        const auto quitter = drogon::WebSocketClient::newWebSocketClient("127.0.0.1", server->port);
        auto request = drogon::HttpRequest::newHttpRequest();
        request->setPath("/ws/docs/" + documentId);
        request->setParameter("token", ownerToken);
        std::promise<void> connected;
        quitter->connectToServer(request, [&](drogon::ReqResult, const drogon::HttpResponsePtr&,
                                              const drogon::WebSocketClientPtr& client) {
            if (const auto connection = client->getConnection()) connection->forceClose();
            connected.set_value();
        });
        connected.get_future().wait();
    }

    const auto next = connect(ownerToken);  // fails here if the room is stuck loading

    next->send(Move{"missing", 1, 1});  // and it really works: the op gets an answer
    EXPECT_TRUE(next->waitUntilSettled(0));
}

// The history lists every op, newest first, with who made it, a page at a time.
TEST_F(SyncHttpTest, TheHistoryListsEveryOpNewestFirstWithItsAuthor) {
    // One at a time: sent at once on two connections, the server could commit them in either order.
    alice->send(rect("a"));
    ASSERT_TRUE(alice->waitUntilSettled(1));
    bob->send(rect("b"));
    ASSERT_TRUE(bob->waitUntilSettled(2));
    alice->send(Move{"a", 1, 2});
    expectEveryoneAgrees({alice.get(), bob.get()});

    std::vector<json> listed;
    std::string before;
    int pages = 0;
    do {
        const Response page =
            send(drogon::Get, "/api/docs/" + documentId + "/ops?limit=2" + before, nullptr, ownerToken);
        ASSERT_EQ(page.status, 200);
        for (const json& op : page.body["ops"]) listed.push_back(op);
        before = page.body["nextBefore"].is_null()
                     ? ""
                     : "&before=" + std::to_string(page.body["nextBefore"].get<int64_t>());
        ++pages;
    } while (!before.empty() && pages < 10);

    ASSERT_EQ(listed.size(), 3u);
    EXPECT_EQ(pages, 2);
    std::vector<int64_t> seqs;
    for (const json& op : listed) seqs.push_back(op["seq"]);
    EXPECT_EQ(seqs, (std::vector<int64_t>{3, 2, 1}));
    EXPECT_EQ(listed[0]["op"], json(Operation{Move{"a", 1, 2}}));
    EXPECT_EQ(listed[0]["userEmail"], "alice@example.com");
    EXPECT_EQ(listed[1]["userEmail"], "bob@example.com");
    EXPECT_TRUE(listed[2]["createdAt"].is_string());
}

TEST_F(SyncHttpTest, BadHistoryParametersAreRejected) {
    const std::string ops = "/api/docs/" + documentId + "/ops";
    EXPECT_EQ(send(drogon::Get, ops + "?limit=lots", nullptr, ownerToken).status, 400);
    EXPECT_EQ(send(drogon::Get, ops + "?before=yesterday", nullptr, ownerToken).status, 400);
    EXPECT_EQ(send(drogon::Get, ops + "?limit=100000", nullptr, ownerToken).status, 200);  // clamped
}

// A version is the newest snapshot from then and the ops after it up to the seq asked for: replayed, the
// drawing as it was.
TEST_F(SyncHttpTest, AVersionReplaysToTheDrawingAsItWasThen) {
    alice->send(rect("a"));
    alice->send(Move{"a", 5, 0});
    // Only once both are in: sent at once, bob's delete could reach the server before "a" exists.
    ASSERT_TRUE(alice->waitUntilSettled(2));
    ASSERT_TRUE(bob->waitUntilSettled(2));
    bob->send(DeleteShape{"a"});
    expectEveryoneAgrees({alice.get(), bob.get()});
    // A snapshot at 2, as the server takes every so often, marked so the test can tell it was used:
    // version 2 starts from it, version 1 from nothing.
    ASSERT_TRUE(drogon::sync_wait(SnapshotRepo(server->database).insert(documentId, 2, {{"marker", 2}})));

    const auto versionAt = [&](int seq) {
        return send(drogon::Get, "/api/docs/" + documentId + "/versions/" + std::to_string(seq), nullptr,
                    editorTokens[0]);
    };
    const Response one = versionAt(1);
    ASSERT_EQ(one.status, 200);
    EXPECT_EQ(one.body["role"], "editor");  // what the reader may do with it
    EXPECT_TRUE(one.body["snapshot"].is_null());
    ASSERT_EQ(one.body["ops"].size(), 1u);
    EXPECT_EQ(one.body["ops"][0]["seq"], 1);
    EXPECT_EQ(one.body["opsComplete"], true);

    const Response two = versionAt(2);
    EXPECT_EQ(two.body["snapshot"], json({{"seq", 2}, {"state", {{"marker", 2}}}}));
    EXPECT_EQ(two.body["ops"], json::array());  // the snapshot is already at 2: no op after it

    const Response three = versionAt(3);
    EXPECT_EQ(three.body["snapshot"]["seq"], 2);
    ASSERT_EQ(three.body["ops"].size(), 1u);
    EXPECT_EQ(three.body["ops"][0]["seq"], 3);

    EXPECT_EQ(versionAt(4).body["error"]["code"], "no_such_version");
    EXPECT_EQ(send(drogon::Get, "/api/docs/" + documentId + "/versions/-1", nullptr, ownerToken).status, 400);
    EXPECT_EQ(send(drogon::Get, "/api/docs/" + documentId + "/versions/latest", nullptr, ownerToken).status,
              400);
}

namespace {

// A raw connection to the document, for what DocClient, which always joins at once, can't do.
struct RawSocket {
    RawSocket(uint16_t port, const std::string& documentId, const std::string& token,
              std::function<void(const json&, const drogon::WebSocketConnectionPtr&)> onMessage) {
        socket = drogon::WebSocketClient::newWebSocketClient("127.0.0.1", port);
        socket->setMessageHandler([onMessage](std::string&& message, const drogon::WebSocketClientPtr& client,
                                              const drogon::WebSocketMessageType& type) {
            const json parsed = json::parse(message, nullptr, false);
            // Control frames, such as the close, aren't JSON objects.
            if (type == drogon::WebSocketMessageType::Text && parsed.is_object())
                onMessage(parsed, client->getConnection());
        });
        socket->setConnectionClosedHandler([this](const drogon::WebSocketClientPtr&) {
            if (!closedOnce.exchange(true)) closed.set_value();
        });
        auto request = drogon::HttpRequest::newHttpRequest();
        request->setPath("/ws/docs/" + documentId);
        request->setParameter("token", token);
        socket->connectToServer(request, [](drogon::ReqResult, const drogon::HttpResponsePtr&,
                                            const drogon::WebSocketClientPtr&) {});
    }
    ~RawSocket() { socket->stop(); }

    drogon::WebSocketClientPtr socket;
    std::atomic<bool> closedOnce = false;
    std::promise<void> closed;
};

}  // namespace

// Postgres drops the connection a joining client's lookup is running on. Only that client is turned
// away: the server, and every other document on it, carry on. Drogon used to end the whole process here.
TEST_F(LiveDocTest, ADatabaseFailureWhileConnectingTurnsAwayOnlyThatConnection) {
    createSharedDocument("alice@example.com", {});
    // A lock the server's role lookup waits on, held on a connection of the test's own.
    const auto locker = drogon::orm::DbClient::newPgClient(*vec::readEnv("TEST_DATABASE_URL"), 1);
    auto lock = locker->newTransaction();
    lock->execSqlSync("LOCK TABLE document_members IN ACCESS EXCLUSIVE MODE");

    auto joining = std::async(std::launch::async, [&] {
        return std::make_unique<DocClient>(server->port, documentId, ownerToken);
    });
    // Once the lookup is waiting on the lock, its connection is killed under it.
    bool killed = false;
    for (int i = 0; i < 200 && !killed; ++i) {
        killed = server->database
                     ->execSqlSync(
                         "SELECT count(pg_terminate_backend(pid)) FROM pg_stat_activity "
                         "WHERE datname = current_database() AND wait_event_type = 'Lock'")[0][0]
                     .as<int64_t>() > 0;
        if (!killed) std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    lock->rollback();
    lock.reset();
    ASSERT_TRUE(killed) << "the lookup never waited on the lock";

    const auto turnedAway = joining.get();
    EXPECT_FALSE(turnedAway->joined());
    EXPECT_NE(turnedAway->problem().find("unavailable"), std::string::npos) << turnedAway->problem();
    EXPECT_EQ(send(drogon::Get, "/healthz").status, 200);  // still running
    const auto next = connect(ownerToken);                 // and the next try works
}

// Before joining, the room may not hold the drawing yet, so an op couldn't be checked against it.
TEST_F(LiveDocTest, AnOpSentBeforeJoiningIsRefused) {
    createSharedDocument("alice@example.com", {});
    std::promise<json> answer;
    std::atomic<bool> answered = false;
    const json op = vec::Operation{vec::AddShape{"a", {RectGeom{0, 0, 10, 10}, {}}, std::nullopt}};
    RawSocket early(
        server->port, documentId, ownerToken,
        [&](const json& message, const drogon::WebSocketConnectionPtr& connection) {
            const std::string type = message.value("type", "");
            if (type == "ready") {  // ready, but not joined: send an op instead of join
                connection->send(
                    json{{"type", "op"}, {"clientOpId", "early"}, {"baseSeq", 0}, {"op", op}}.dump());
            } else if (type == "reject" && !answered.exchange(true)) {
                answer.set_value(message);
            }
        });

    auto future = answer.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_EQ(future.get()["reason"], "not_joined");
    EXPECT_EQ(headSeq(), 0);
}

// A connection lasts no longer than its token, so access that has been taken away doesn't linger.
TEST_F(LiveDocTest, AConnectionClosesWhenItsTokenExpires) {
    createSharedDocument("alice@example.com", {});
    const std::string userId = send(drogon::Get, "/api/me", nullptr, ownerToken).body["id"];
    // Issued so as to expire two seconds from now.
    const auto issued =
        std::chrono::system_clock::now() - vec::AccessTokens::lifetime + std::chrono::seconds(2);
    const std::string shortLived = vec::AccessTokens(server->jwtSecret).issue(userId, issued);

    RawSocket expiring(server->port, documentId, shortLived,
                       [](const json& message, const drogon::WebSocketConnectionPtr& connection) {
                           if (message.value("type", "") == "ready")
                               connection->send(json{{"type", "join"}, {"lastSeq", 0}}.dump());
                       });

    // The heartbeat that checks runs every 5 s.
    EXPECT_EQ(expiring.closed.get_future().wait_for(std::chrono::seconds(12)), std::future_status::ready);
}
