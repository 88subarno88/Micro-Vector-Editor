#include <drogon/utils/Utilities.h>
#include <gtest/gtest.h>
#include <trantor/net/EventLoopThread.h>

#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "collab/Room.h"

using namespace vec;

namespace {

// A member that records what it was sent.
struct Inbox {
    std::vector<std::string> messages;
    RoomMember member(const std::string& userId) {
        return {userId, [this](const std::string& message) { messages.push_back(message); }};
    }
};

class RoomRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        for (int i = 0; i < 4; ++i) {
            threads.push_back(std::make_unique<trantor::EventLoopThread>());
            threads.back()->run();
            loops.push_back(threads.back()->getLoop());
        }
    }

    std::vector<std::unique_ptr<trantor::EventLoopThread>> threads;
    std::vector<trantor::EventLoop*> loops;
};

}  // namespace

TEST(RoomTest, BroadcastReachesEveryMember) {
    Room room("doc", nullptr);
    Inbox a, b;
    room.join(1, a.member("ann"));
    room.join(2, b.member("bob"));

    room.broadcast("hello");

    EXPECT_EQ(a.messages, std::vector<std::string>{"hello"});
    EXPECT_EQ(b.messages, std::vector<std::string>{"hello"});
}

TEST(RoomTest, BroadcastCanSkipTheSender) {
    Room room("doc", nullptr);
    Inbox a, b;
    room.join(1, a.member("ann"));
    room.join(2, b.member("bob"));

    room.broadcast("from ann", 1);

    EXPECT_TRUE(a.messages.empty());
    EXPECT_EQ(b.messages, std::vector<std::string>{"from ann"});
}

TEST(RoomTest, OneUserCanHaveSeveralSessions) {
    Room room("doc", nullptr);
    Inbox laptop, phone;
    room.join(1, laptop.member("ann"));
    room.join(2, phone.member("ann"));

    room.broadcast("x");

    EXPECT_EQ(room.memberCount(), 2u);
    EXPECT_EQ(laptop.messages.size(), 1u);
    EXPECT_EQ(phone.messages.size(), 1u);
}

TEST(RoomTest, LeftMembersGetNothing) {
    Room room("doc", nullptr);
    Inbox a, b;
    room.join(1, a.member("ann"));
    room.join(2, b.member("bob"));

    room.leave(1);
    room.leave(99);  // unknown: ignored
    room.broadcast("x");

    EXPECT_EQ(room.memberCount(), 1u);
    EXPECT_TRUE(a.messages.empty());
    EXPECT_EQ(b.messages.size(), 1u);
}

TEST(RoomTest, ANewRoomIsEmptyAtTheStartOfTheLog) {
    Room room("doc", nullptr);

    EXPECT_EQ(room.memberCount(), 0u);
    EXPECT_EQ(room.lastSeq(), 0);
    EXPECT_EQ(room.diagram().getObjectCount(), 0);
}

TEST_F(RoomRegistryTest, OneDocumentAlwaysGetsTheSameRoomAndLoop) {
    RoomRegistry registry(loops.size(), [this](size_t i) { return loops[i]; });
    const auto first = registry.roomFor("doc-1");

    EXPECT_EQ(registry.roomFor("doc-1"), first);
    EXPECT_EQ(first->loop(), registry.loopFor("doc-1"));
    EXPECT_EQ(first->documentId(), "doc-1");
    EXPECT_EQ(registry.roomCount(), 1u);
}

TEST_F(RoomRegistryTest, DocumentsAreSpreadOverEveryLoop) {
    RoomRegistry registry(loops.size(), [this](size_t i) { return loops[i]; });
    std::set<trantor::EventLoop*> used;
    for (int i = 0; i < 200; ++i) used.insert(registry.roomFor(drogon::utils::getUuid())->loop());

    EXPECT_EQ(used.size(), loops.size());
    EXPECT_EQ(registry.roomCount(), 200u);
}

TEST_F(RoomRegistryTest, ConcurrentLookupsOfOneDocumentShareOneRoom) {
    RoomRegistry registry(loops.size(), [this](size_t i) { return loops[i]; });
    std::vector<std::shared_ptr<Room>> found(8);
    std::vector<std::thread> threadsLookingUp;
    for (size_t i = 0; i < found.size(); ++i) {
        threadsLookingUp.emplace_back([&, i] { found[i] = registry.roomFor("shared"); });
    }
    for (std::thread& thread : threadsLookingUp) thread.join();

    for (const auto& room : found) EXPECT_EQ(room, found[0]);
    EXPECT_EQ(registry.roomCount(), 1u);
}
