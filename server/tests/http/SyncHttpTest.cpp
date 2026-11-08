#include <memory>
#include <string>

#include "DocClient.h"
#include "HttpTest.h"
#include "collab/LoadDrawing.h"
#include "ops/Json.h"

using namespace vec;
using nlohmann::json;

namespace {

// A document shared by alice and bob, both editors, each with a live connection to it.
class SyncHttpTest : public HttpTest {
protected:
    void SetUp() override {
        HttpTest::SetUp();
        if (IsSkipped()) return;
        aliceToken = signUpAndLogIn("alice@example.com").accessToken;
        bobToken = signUpAndLogIn("bob@example.com").accessToken;
        documentId = send(drogon::Post, "/api/docs", {{"title", "shared"}}, aliceToken).body["id"];
        ASSERT_EQ(send(drogon::Post, "/api/docs/" + documentId + "/members",
                       {{"email", "bob@example.com"}, {"role", "editor"}}, aliceToken)
                      .status,
                  201);
        alice = connect(aliceToken);
        bob = connect(bobToken);
    }

    std::unique_ptr<DocClient> connect(const std::string& token) {
        auto client = std::make_unique<DocClient>(server->port, documentId, token);
        EXPECT_TRUE(client->joined()) << client->problem();
        return client;
    }

    static AddShape rect(const std::string& id) { return {id, {RectGeom{0, 0, 10, 10}, {}}, std::nullopt}; }
    static AddShape circle(const std::string& id) {
        return {id, {CircleGeom{{50, 50}, 5}, {}}, std::nullopt};
    }

    int64_t headSeq() {
        return send(drogon::Get, "/api/docs/" + documentId, nullptr, aliceToken).body["document"]["headSeq"];
    }

    // The drawing as the server would load it now, from Postgres.
    json serverDrawing() {
        const auto drawing = drogon::sync_wait(loadDrawing(server->database, documentId));
        EXPECT_TRUE(drawing) << drawing.error();
        return drawing ? diagramToJson(drawing->diagram) : json();
    }

    // Waits until every client has its answers and has heard of every committed op, then checks they all
    // hold the drawing the server would load.
    void expectEveryoneAgrees(std::initializer_list<DocClient*> clients) {
        // First every answer, so that the head read next is final.
        for (DocClient* client : clients) ASSERT_TRUE(client->waitUntilSettled(0)) << "unanswered ops";
        const int64_t head = headSeq();
        const json expected = serverDrawing();
        for (DocClient* client : clients) {
            ASSERT_TRUE(client->waitUntilSettled(head))
                << "stuck at seq " << client->lastSeq() << " of " << head;
            EXPECT_EQ(client->problem(), "");
            EXPECT_EQ(client->replica(), expected);
        }
    }

    std::string aliceToken, bobToken, documentId;
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

    const auto late = connect(bobToken);

    expectEveryoneAgrees({alice.get(), bob.get(), late.get()});
}
