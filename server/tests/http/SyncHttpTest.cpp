#include <drogon/HttpRequest.h>
#include <drogon/WebSocketClient.h>

#include <future>
#include <memory>
#include <string>

#include "LiveDocTest.h"

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
    alice->send(rect("a"));
    bob->send(rect("b"));
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
