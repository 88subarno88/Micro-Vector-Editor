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
