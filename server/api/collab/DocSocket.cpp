#include "collab/DocSocket.h"

#include <drogon/utils/coroutine.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

#include "collab/LoadDrawing.h"
#include "db/DocRepo.h"
#include "logging/Log.h"

using drogon::CloseCode;
using drogon::WebSocketConnectionPtr;
using nlohmann::json;

namespace {

// Application close codes live in 4000-4999; these echo the HTTP statuses they stand for.
constexpr auto closeUnauthorized = static_cast<CloseCode>(4401);
constexpr auto closeNotFound = static_cast<CloseCode>(4404);

constexpr std::string_view pathPrefix = "/ws/docs/";

std::atomic<vec::SessionId> nextSessionId = 1;

void sendJson(const WebSocketConnectionPtr& connection, const json& message) {
    connection->send(message.dump());
}

}  // namespace

// One connection's identity and room. Created on the connection's thread, joined to a room on the room's,
// and closed on the connection's again, so the fields shared between threads are atomic or locked.
struct DocSession {
    vec::SessionId id = 0;
    std::string userId;
    std::string documentId;
    vec::Role role = vec::Role::Viewer;  // written before `ready` is set, read only after
    std::atomic<bool> ready = false;     // a member of `room`
    std::atomic<bool> closed = false;

    std::shared_ptr<vec::Room> room() {
        const std::lock_guard lock(mutex);
        return room_;
    }
    void setRoom(std::shared_ptr<vec::Room> room) {
        const std::lock_guard lock(mutex);
        room_ = std::move(room);
    }

private:
    std::mutex mutex;
    std::shared_ptr<vec::Room> room_;
};

void DocSocket::handleNewConnection(const drogon::HttpRequestPtr& request,
                                    const WebSocketConnectionPtr& connection) {
    std::string token = request->getParameter("token");
    if (token.empty())
        token = std::string(vec::bearerToken(request->getHeader("authorization")).value_or(""));
    const auto userId = tokens_.verify(token);
    if (!userId) return connection->shutdown(closeUnauthorized, "unauthorized");

    auto session = std::make_shared<DocSession>();
    session->id = nextSessionId++;
    session->userId = *userId;
    session->documentId = request->path().substr(pathPrefix.size());
    connection->setContext(session);

    drogon::async_run([this, connection, session]() -> drogon::Task<> {
        const auto role = co_await vec::MemberRepo(database_).roleOf(session->documentId, session->userId);
        if (!role) {
            connection->shutdown(closeNotFound, "not_found");
            co_return;
        }
        session->role = *role;
        joinRoom(connection, session);
    });
}

void DocSocket::joinRoom(const WebSocketConnectionPtr& connection,
                         const std::shared_ptr<DocSession>& session) {
    const auto room = rooms_->roomFor(session->documentId);
    room->loop()->queueInLoop([this, connection, session, room] {
        // A weak pointer, so that the room doesn't keep a closed connection alive.
        const auto result = room->join(
            session->id, {session->userId, [weak = std::weak_ptr(connection)](const std::string& message) {
                              if (const auto open = weak.lock()) open->send(message);
                          }});
        if (result == vec::Room::JoinResult::Unloaded) return joinRoom(connection, session);  // a fresh room

        session->setRoom(room);
        session->ready = true;
        // If the connection closed while joining, its close handler saw `ready` false and left this to us.
        if (session->closed) return leaveRoom(session);

        vec::logInfo("joined room", {},
                     {{"document", session->documentId}, {"members", room->memberCount()}});
        if (result == vec::Room::JoinResult::JoinedAndMustLoad) loadRoom(room);
        sendJson(connection, {{"type", "ready"}, {"role", vec::toString(session->role)}});
    });
}

void DocSocket::loadRoom(const std::shared_ptr<vec::Room>& room) {
    drogon::async_run([this, room]() -> drogon::Task<> {
        const auto started = std::chrono::steady_clock::now();
        std::shared_ptr<vec::LoadedDrawing> loaded;
        std::string error;
        try {
            auto result = co_await vec::loadDrawing(database_, room->documentId());
            if (result)
                loaded = std::make_shared<vec::LoadedDrawing>(std::move(*result));
            else
                error = result.error();
        } catch (const std::exception& e) {
            error = e.what();
        }
        const auto millis =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started)
                .count();

        room->loop()->queueInLoop([room, loaded, error, millis] {
            if (!loaded) {
                room->failLoad();
                vec::logError("room load failed", {}, {{"document", room->documentId()}, {"error", error}});
                return;
            }
            room->finishLoad(std::move(loaded->diagram), loaded->lastSeq);
            vec::logInfo("loaded room", {},
                         {{"document", room->documentId()},
                          {"lastSeq", room->lastSeq()},
                          {"shapes", room->diagram().getObjectCount()},
                          {"ms", millis}});
        });
    });
}

void DocSocket::leaveRoom(const std::shared_ptr<DocSession>& session) {
    const auto room = session->room();
    room->loop()->queueInLoop([this, session, room] {
        const auto emptied = room->leave(session->id);
        vec::logInfo("left room", {}, {{"document", session->documentId}, {"members", room->memberCount()}});
        if (!emptied) return;

        room->loop()->runAfter(unloadDelay_.count(), [this, room, token = *emptied] {
            if (!room->unloadIfStillEmpty(token)) return;
            rooms_->remove(room);
            vec::logInfo("unloaded room", {}, {{"document", room->documentId()}});
        });
    });
}

void DocSocket::handleNewMessage(const WebSocketConnectionPtr& connection, std::string&& message,
                                 const drogon::WebSocketMessageType& type) {
    if (type != drogon::WebSocketMessageType::Text) return;  // pings are answered by Drogon itself
    const auto session = connection->getContext<DocSession>();
    if (!session || !session->ready) {
        return sendJson(connection, {{"type", "error"}, {"code", "not_ready"}});
    }
    connection->send(message);
}

void DocSocket::handleConnectionClosed(const WebSocketConnectionPtr& connection) {
    const auto session = connection->getContext<DocSession>();
    if (!session) return;
    session->closed = true;
    // Not joined yet: the join will see `closed` and leave by itself.
    if (session->ready) leaveRoom(session);
}
