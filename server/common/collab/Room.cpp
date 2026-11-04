#include "collab/Room.h"

namespace vec {

void Room::join(SessionId session, RoomMember member) {
    members_.insert_or_assign(session, std::move(member));
}

void Room::leave(SessionId session) { members_.erase(session); }

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

size_t RoomRegistry::roomCount() const {
    const std::lock_guard lock(mutex_);
    return rooms_.size();
}

}  // namespace vec
