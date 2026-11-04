#include "collab/Room.h"

namespace vec {

Room::JoinResult Room::join(SessionId session, RoomMember member) {
    if (unloaded_) return JoinResult::Unloaded;
    members_.insert_or_assign(session, std::move(member));
    if (loadState_ == LoadState::NotLoaded || loadState_ == LoadState::Failed) {
        loadState_ = LoadState::Loading;
        return JoinResult::JoinedAndMustLoad;
    }
    return JoinResult::Joined;
}

std::optional<uint64_t> Room::leave(SessionId session) {
    if (members_.erase(session) == 0 || !members_.empty()) return std::nullopt;
    return ++emptiedCount_;
}

bool Room::unloadIfStillEmpty(uint64_t token) {
    if (!members_.empty() || token != emptiedCount_ || unloaded_) return false;
    unloaded_ = true;
    return true;
}

void Room::finishLoad(Diagram diagram, int64_t lastSeq) {
    diagram_ = std::move(diagram);
    lastSeq_ = lastSeq;
    loadState_ = LoadState::Loaded;
}

void Room::failLoad() { loadState_ = LoadState::Failed; }

void Room::broadcast(const std::string& message, std::optional<SessionId> skip) const {
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
