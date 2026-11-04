#include <drogon/utils/Utilities.h>
#include <gtest/gtest.h>
#include <trantor/net/EventLoopThread.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
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

// Room methods may only run on the room's loop, so each test body runs there.
class RoomTest : public ::testing::Test {
protected:
    void SetUp() override { thread.run(); }

    trantor::EventLoop* loop() { return thread.getLoop(); }

    // Runs `body` on the loop and waits for it to finish.
    void onLoop(const std::function<void()>& body) {
        std::promise<void> done;
        loop()->queueInLoop([&] {
            body();
            done.set_value();
        });
        done.get_future().wait();
    }

    trantor::EventLoopThread thread;
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

TEST_F(RoomTest, BroadcastReachesEveryMember) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a, b;
        room.join(1, a.member("ann"));
        room.join(2, b.member("bob"));

        room.broadcast("hello");

        EXPECT_EQ(a.messages, std::vector<std::string>{"hello"});
        EXPECT_EQ(b.messages, std::vector<std::string>{"hello"});
    });
}

TEST_F(RoomTest, BroadcastCanSkipTheSender) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a, b;
        room.join(1, a.member("ann"));
        room.join(2, b.member("bob"));

        room.broadcast("from ann", 1);

        EXPECT_TRUE(a.messages.empty());
        EXPECT_EQ(b.messages, std::vector<std::string>{"from ann"});
    });
}

TEST_F(RoomTest, OneUserCanHaveSeveralSessions) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox laptop, phone;
        room.join(1, laptop.member("ann"));
        room.join(2, phone.member("ann"));

        room.broadcast("x");

        EXPECT_EQ(room.memberCount(), 2u);
        EXPECT_EQ(laptop.messages.size(), 1u);
        EXPECT_EQ(phone.messages.size(), 1u);
    });
}

TEST_F(RoomTest, LeftMembersGetNothing) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a, b;
        room.join(1, a.member("ann"));
        room.join(2, b.member("bob"));

        room.leave(1);
        room.leave(99);  // unknown: ignored
        room.broadcast("x");

        EXPECT_EQ(room.memberCount(), 1u);
        EXPECT_TRUE(a.messages.empty());
        EXPECT_EQ(b.messages.size(), 1u);
    });
}

TEST_F(RoomTest, ANewRoomIsEmptyAtTheStartOfTheLog) {
    onLoop([&] {
        Room room("doc", loop());

        EXPECT_EQ(room.memberCount(), 0u);
        EXPECT_EQ(room.lastSeq(), 0);
        EXPECT_EQ(room.diagram().getObjectCount(), 0);
    });
}

TEST_F(RoomTest, OnlyTheFirstJoinLoads) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a, b;

        EXPECT_EQ(room.join(1, a.member("ann")), Room::JoinResult::JoinedAndMustLoad);
        EXPECT_EQ(room.loadState(), Room::LoadState::Loading);
        EXPECT_EQ(room.join(2, b.member("bob")), Room::JoinResult::Joined);  // while still loading
    });
}

TEST_F(RoomTest, FinishLoadInstallsTheDrawing) {
    onLoop([&] {
        Room room("doc", loop());
        Diagram diagram;
        diagram.addObject(std::make_unique<Rectangle>(1, 2, 3, 4));

        room.finishLoad(std::move(diagram), 42);

        EXPECT_EQ(room.loadState(), Room::LoadState::Loaded);
        EXPECT_EQ(room.lastSeq(), 42);
        EXPECT_EQ(room.diagram().getObjectCount(), 1);
    });
}

TEST_F(RoomTest, AFailedLoadIsRetriedByTheNextJoin) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a, b;
        ASSERT_EQ(room.join(1, a.member("ann")), Room::JoinResult::JoinedAndMustLoad);

        room.failLoad();

        EXPECT_EQ(room.join(2, b.member("bob")), Room::JoinResult::JoinedAndMustLoad);
    });
}

TEST_F(RoomTest, LeavingReturnsATokenOnlyWhenTheRoomEmpties) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a, b;
        room.join(1, a.member("ann"));
        room.join(2, b.member("bob"));

        EXPECT_EQ(room.leave(1), std::nullopt);
        EXPECT_TRUE(room.leave(2));
        EXPECT_EQ(room.leave(2), std::nullopt);  // already gone
    });
}

TEST_F(RoomTest, AnEmptyRoomUnloadsAndRefusesJoins) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a;
        room.join(1, a.member("ann"));
        const auto token = room.leave(1);

        EXPECT_TRUE(room.unloadIfStillEmpty(*token));
        EXPECT_FALSE(room.unloadIfStillEmpty(*token));
        EXPECT_EQ(room.join(2, a.member("ann")), Room::JoinResult::Unloaded);
    });
}

TEST_F(RoomTest, SomeoneJoiningCancelsAnUnloadThatWasDue) {
    onLoop([&] {
        Room room("doc", loop());
        Inbox a;
        room.join(1, a.member("ann"));
        const auto token = room.leave(1);
        room.join(2, a.member("ann"));

        EXPECT_FALSE(room.unloadIfStillEmpty(*token));
        EXPECT_EQ(room.memberCount(), 1u);
    });
}

TEST_F(RoomTest, OnlyTheLatestEmptyingCanUnload) {
    onLoop([&] {
        // leave, join, leave: the first unload timer fires while the room is empty again, but too early.
        Room room("doc", loop());
        Inbox a;
        room.join(1, a.member("ann"));
        const auto first = room.leave(1);
        room.join(2, a.member("ann"));
        const auto second = room.leave(2);

        EXPECT_FALSE(room.unloadIfStillEmpty(*first));
        EXPECT_TRUE(room.unloadIfStillEmpty(*second));
    });
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

TEST_F(RoomTest, UsingARoomOffItsLoopAborts) {
    Room room("doc", loop());
    Inbox inbox;

    EXPECT_DEATH(room.join(1, inbox.member("ann")), "");
}

namespace {

// Waits for `count` to reach `target`, failing after a few seconds.
void waitFor(const std::atomic<int>& count, int target) {
    for (int i = 0; i < 300 && count < target; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(count, target);
}

}  // namespace

TEST_F(RoomTest, TasksRunInOrderOneAtATime) {
    const auto room = std::make_shared<Room>("doc", loop());
    trantor::EventLoopThread elsewhere;  // tasks finish on another thread, as database work does
    elsewhere.run();
    std::vector<int> order;
    std::atomic<int> running = 0, maxRunning = 0, finished = 0;

    onLoop([&] {
        for (int i = 0; i < 20; ++i) {
            ASSERT_TRUE(room->enqueue([&, i](std::function<void()> done) {
                order.push_back(i);
                maxRunning = std::max(maxRunning.load(), ++running);
                elsewhere.getLoop()->runAfter(0.002, [&, done] {
                    --running;
                    ++finished;
                    done();
                });
            }));
        }
    });
    waitFor(finished, 20);

    std::vector<int> expected(20);
    std::iota(expected.begin(), expected.end(), 0);
    EXPECT_EQ(order, expected);
    EXPECT_EQ(maxRunning, 1);
}

TEST_F(RoomTest, TheNextTaskWaitsForDone) {
    const auto room = std::make_shared<Room>("doc", loop());
    std::function<void()> finishFirst;
    std::atomic<int> started = 0;

    onLoop([&] {
        room->enqueue([&](std::function<void()> done) {
            ++started;
            finishFirst = done;
        });
        room->enqueue([&](std::function<void()> done) {
            ++started;
            done();
        });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(started, 1);

    finishFirst();
    waitFor(started, 2);
}

TEST_F(RoomTest, CallingDoneTwiceStartsOnlyOneNextTask) {
    const auto room = std::make_shared<Room>("doc", loop());
    std::atomic<int> started = 0;
    std::function<void()> second;

    onLoop([&] {
        room->enqueue([&](std::function<void()> done) {
            ++started;
            done();
            done();
        });
        room->enqueue([&](std::function<void()> done) {
            ++started;
            second = done;  // never finished
        });
        room->enqueue([&](std::function<void()> done) {
            ++started;
            done();
        });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(started, 2);
}

TEST_F(RoomTest, ATaskThatThrowsDoesNotStallTheQueue) {
    const auto room = std::make_shared<Room>("doc", loop());
    std::atomic<int> after = 0;

    onLoop([&] {
        room->enqueue([](std::function<void()>) { throw std::runtime_error("boom"); });
        room->enqueue([&](std::function<void()> done) {
            ++after;
            done();
        });
    });

    waitFor(after, 1);
}

TEST_F(RoomTest, AFullQueueRefusesMoreTasks) {
    const auto room = std::make_shared<Room>("doc", loop());
    std::function<void()> finishFirst;

    onLoop([&] {
        room->enqueue([&](std::function<void()> done) { finishFirst = done; });  // runs, and holds the queue
        for (size_t i = 0; i < Room::maxQueuedTasks; ++i) {
            ASSERT_TRUE(room->enqueue([](std::function<void()> done) { done(); }));
        }
        EXPECT_FALSE(room->enqueue([](std::function<void()> done) { done(); }));
        EXPECT_EQ(room->queuedTasks(), Room::maxQueuedTasks);
    });
    finishFirst();
}

TEST_F(RoomTest, ARoomWithWorkLeftDoesNotUnload) {
    const auto room = std::make_shared<Room>("doc", loop());
    Inbox a;
    std::function<void()> finishTask;

    onLoop([&] {
        room->join(1, a.member("ann"));
        room->enqueue([&](std::function<void()> done) { finishTask = done; });
        const auto token = room->leave(1);
        EXPECT_FALSE(room->unloadIfStillEmpty(*token));
    });
    finishTask();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    onLoop([&] { EXPECT_EQ(room->queuedTasks(), 0u); });
}
