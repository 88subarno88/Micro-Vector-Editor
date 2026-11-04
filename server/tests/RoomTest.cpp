#include <drogon/utils/Utilities.h>
#include <gtest/gtest.h>
#include <trantor/net/EventLoopThread.h>

#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "collab/Room.h"
#include "core/Rectangle.h"

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

TEST(RoomTest, OnlyTheFirstJoinLoads) {
    Room room("doc", nullptr);
    Inbox a, b;

    EXPECT_EQ(room.join(1, a.member("ann")), Room::JoinResult::JoinedAndMustLoad);
    EXPECT_EQ(room.loadState(), Room::LoadState::Loading);
    EXPECT_EQ(room.join(2, b.member("bob")), Room::JoinResult::Joined);  // while still loading
}

TEST(RoomTest, FinishLoadInstallsTheDrawing) {
    Room room("doc", nullptr);
    Diagram diagram;
    diagram.addObject(std::make_unique<Rectangle>(1, 2, 3, 4));

    room.finishLoad(std::move(diagram), 42);

    EXPECT_EQ(room.loadState(), Room::LoadState::Loaded);
    EXPECT_EQ(room.lastSeq(), 42);
    EXPECT_EQ(room.diagram().getObjectCount(), 1);
}

TEST(RoomTest, AFailedLoadIsRetriedByTheNextJoin) {
    Room room("doc", nullptr);
    Inbox a, b;
    ASSERT_EQ(room.join(1, a.member("ann")), Room::JoinResult::JoinedAndMustLoad);

    room.failLoad();

    EXPECT_EQ(room.join(2, b.member("bob")), Room::JoinResult::JoinedAndMustLoad);
}

TEST(RoomTest, LeavingReturnsATokenOnlyWhenTheRoomEmpties) {
    Room room("doc", nullptr);
    Inbox a, b;
    room.join(1, a.member("ann"));
    room.join(2, b.member("bob"));

    EXPECT_EQ(room.leave(1), std::nullopt);
    EXPECT_TRUE(room.leave(2));
    EXPECT_EQ(room.leave(2), std::nullopt);  // already gone
}

TEST(RoomTest, AnEmptyRoomUnloadsAndRefusesJoins) {
    Room room("doc", nullptr);
    Inbox a;
    room.join(1, a.member("ann"));
    const auto token = room.leave(1);

    EXPECT_TRUE(room.unloadIfStillEmpty(*token));
    EXPECT_FALSE(room.unloadIfStillEmpty(*token));
    EXPECT_EQ(room.join(2, a.member("ann")), Room::JoinResult::Unloaded);
}

TEST(RoomTest, SomeoneJoiningCancelsAnUnloadThatWasDue) {
    Room room("doc", nullptr);
    Inbox a;
    room.join(1, a.member("ann"));
    const auto token = room.leave(1);
    room.join(2, a.member("ann"));

    EXPECT_FALSE(room.unloadIfStillEmpty(*token));
    EXPECT_EQ(room.memberCount(), 1u);
}

TEST(RoomTest, OnlyTheLatestEmptyingCanUnload) {
    // leave, join, leave: the first unload timer fires while the room is empty again, but too early.
    Room room("doc", nullptr);
    Inbox a;
    room.join(1, a.member("ann"));
    const auto first = room.leave(1);
    room.join(2, a.member("ann"));
    const auto second = room.leave(2);

    EXPECT_FALSE(room.unloadIfStillEmpty(*first));
    EXPECT_TRUE(room.unloadIfStillEmpty(*second));
}

TEST_F(RoomRegistryTest, RemovingARoomGivesItsDocumentAFreshOne) {
    RoomRegistry registry(loops.size(), [this](size_t i) { return loops[i]; });
    const auto old = registry.roomFor("doc");

    registry.remove(old);

    EXPECT_EQ(registry.roomCount(), 0u);
    EXPECT_NE(registry.roomFor("doc"), old);
}

TEST_F(RoomRegistryTest, RemovingAStaleRoomLeavesTheCurrentOneAlone) {
    RoomRegistry registry(loops.size(), [this](size_t i) { return loops[i]; });
    const auto old = registry.roomFor("doc");
    registry.remove(old);
    const auto current = registry.roomFor("doc");

    registry.remove(old);

    EXPECT_EQ(registry.roomFor("doc"), current);
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
