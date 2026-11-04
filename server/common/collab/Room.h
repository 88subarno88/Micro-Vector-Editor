#ifndef SERVER_ROOM_H
#define SERVER_ROOM_H

#include <trantor/net/EventLoop.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/Diagram.h"

namespace vec {

// One connected client, as far as a room is concerned.
struct RoomMember {
    std::string userId;
    std::function<void(const std::string&)> send;  // thread-safe: Drogon queues it onto the connection
};

using SessionId = uint64_t;

// Everything live about one open document: its drawing, how far into the op log that drawing is, and
// who is connected. A room is pinned to one event loop, and all of its state is only touched on that
// loop's thread, so it needs no locks: post work to it with loop()->queueInLoop.
class Room {
public:
    Room(std::string documentId, trantor::EventLoop* loop)
        : documentId_(std::move(documentId)), loop_(loop) {}

    const std::string& documentId() const { return documentId_; }
    trantor::EventLoop* loop() const { return loop_; }

    enum class JoinResult {
        Joined,
        JoinedAndMustLoad,  // the first join: the caller loads the drawing, then calls finishLoad()
        Unloaded,           // this room was unloaded meanwhile; join the registry's current room instead
    };
    JoinResult join(SessionId session, RoomMember member);

    // Returns a token for scheduling an unload if this left the room empty, so that only the most recent
    // emptying can unload it: someone may join and leave again before the first unload is due.
    std::optional<uint64_t> leave(SessionId session);

    // Marks the room unloaded if it's still empty since `token`; true if it did. Joins then fail.
    bool unloadIfStillEmpty(uint64_t token);

    enum class LoadState { NotLoaded, Loading, Loaded, Failed };
    LoadState loadState() const { return loadState_; }

    // Installs the drawing loaded for the first join, at `lastSeq` in the op log, or records a failure so
    // that the next join tries again.
    void finishLoad(Diagram diagram, int64_t lastSeq);
    void failLoad();

    // Sends `message` to every member, except `skip` if given.
    void broadcast(const std::string& message, std::optional<SessionId> skip = std::nullopt) const;

    size_t memberCount() const { return members_.size(); }
    Diagram& diagram() { return diagram_; }
    int64_t lastSeq() const { return lastSeq_; }

private:
    std::string documentId_;
    trantor::EventLoop* loop_;
    Diagram diagram_;
    int64_t lastSeq_ = 0;  // the seq of the last op applied to diagram_
    std::unordered_map<SessionId, RoomMember> members_;
    LoadState loadState_ = LoadState::NotLoaded;
    uint64_t emptiedCount_ = 0;  // increases each time the room becomes empty
    bool unloaded_ = false;
};

// Finds or creates the room for a document. Rooms are spread over `loopCount` loops by a hash of the
// document ID, so one document always lands on the same loop and its work is serialized there, while
// different documents run in parallel. `loopAt` is only called once rooms are needed, as Drogon creates
// its IO loops when it starts running. Safe to call from any thread.
class RoomRegistry {
public:
    RoomRegistry(size_t loopCount, std::function<trantor::EventLoop*(size_t)> loopAt)
        : loopCount_(loopCount), loopAt_(std::move(loopAt)) {}

    std::shared_ptr<Room> roomFor(const std::string& documentId);

    // The loop a document's room runs on.
    trantor::EventLoop* loopFor(const std::string& documentId) const;

    size_t roomCount() const;

    // Forgets `room`, if it is still the room for its document.
    void remove(const std::shared_ptr<Room>& room);

private:
    size_t loopCount_;
    std::function<trantor::EventLoop*(size_t)> loopAt_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Room>> rooms_;
};

}  // namespace vec

#endif  // SERVER_ROOM_H
