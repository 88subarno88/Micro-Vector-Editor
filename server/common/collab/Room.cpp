#include "collab/Room.h"

#include <atomic>

#include "logging/Log.h"
#include "ops/Apply.h"

namespace vec {

Room::JoinResult Room::join(SessionId session, RoomMember member) {
    assertOnLoop();
    if (unloaded_) return JoinResult::Unloaded;
    members_.insert_or_assign(session, std::move(member));
    if (loadState_ == LoadState::NotLoaded || loadState_ == LoadState::Failed) {
        loadState_ = LoadState::Loading;
        return JoinResult::JoinedAndMustLoad;
    }
    return JoinResult::Joined;
}

std::optional<uint64_t> Room::leave(SessionId session) {
    assertOnLoop();
    if (members_.erase(session) == 0 || !members_.empty()) return std::nullopt;
    return ++emptiedCount_;
}

bool Room::unloadIfStillEmpty(uint64_t token) {
    assertOnLoop();
    if (!members_.empty() || token != emptiedCount_ || unloaded_ || taskRunning_ || !pending_.empty())
        return false;
    unloaded_ = true;
    return true;
}

void Room::finishLoad(Diagram diagram, int64_t lastSeq) {
    assertOnLoop();
    diagram_ = std::move(diagram);
    lastSeq_ = lastSeq;
    loadState_ = LoadState::Loaded;
}

bool Room::applyCommitted(const Operation& op, int64_t seq) {
    assertOnLoop();
    if (seq != lastSeq_ + 1 || !vec::apply(diagram_, op)) return false;
    lastSeq_ = seq;
    return true;
}

void Room::failLoad() {
    assertOnLoop();
    loadState_ = LoadState::Failed;
}

bool Room::enqueue(Task task) {
    assertOnLoop();
    if (pending_.size() >= maxQueuedTasks) return false;
    pending_.push_back(std::move(task));
    runNextTask();
    return true;
}

void Room::runNextTask() {
    if (taskRunning_ || pending_.empty()) return;
    taskRunning_ = true;
    Task task = std::move(pending_.front());
    pending_.pop_front();

    auto finished = std::make_shared<std::atomic<bool>>(false);
    auto done = [self = shared_from_this(), finished] {
        if (finished->exchange(true)) return;
        self->loop_->queueInLoop([self] {
            self->taskRunning_ = false;
            self->runNextTask();
        });
    };
    try {
        task(done);
    } catch (const std::exception& e) {
        logError("room task threw", {}, {{"document", documentId_}, {"error", e.what()}});
        done();
    }
}

void Room::broadcast(const std::string& message, std::optional<SessionId> skip) const {
    assertOnLoop();
    for (const auto& [session, member] : members_) {
        if (session != skip) member.send(message);
    }
}

std::shared_ptr<Room> RoomRegistry::roomFor(const std::string& documentId) {
    const std::lock_guard lock(mutex_);
    auto& room = rooms_[documentId];
    if (!room) room = std::make_shared<Room>(documentId, loopFor(documentId));
    return room;
}

trantor::EventLoop* RoomRegistry::loopFor(const std::string& documentId) const {
    return loopAt_(std::hash<std::string>{}(documentId) % loopCount_);
}

void RoomRegistry::remove(const std::shared_ptr<Room>& room) {
    const std::lock_guard lock(mutex_);
    const auto it = rooms_.find(room->documentId());
    if (it != rooms_.end() && it->second == room) rooms_.erase(it);
}

size_t RoomRegistry::roomCount() const {
    const std::lock_guard lock(mutex_);
    return rooms_.size();
}

}  // namespace vec
