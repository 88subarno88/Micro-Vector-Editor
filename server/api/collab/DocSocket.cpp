#include "collab/DocSocket.h"

#include <drogon/utils/coroutine.h>

#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

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

// One connection's identity. Created before the role is known, on the connection's own thread, and
// marked ready by the database callback on another: `ready` is the hand-over between the two.
struct Session {
    vec::SessionId id = 0;
    std::string userId;
    std::string documentId;
    std::shared_ptr<vec::Room> room;     // set before `ready`
    vec::Role role = vec::Role::Viewer;  // written before `ready` is set, read only after
    std::atomic<bool> ready = false;
};

std::atomic<vec::SessionId> nextSessionId = 1;

void sendJson(const WebSocketConnectionPtr& connection, const json& message) {
    connection->send(message.dump());
}

}  // namespace

void DocSocket::handleNewConnection(const drogon::HttpRequestPtr& request,
                                    const WebSocketConnectionPtr& connection) {
    std::string token = request->getParameter("token");
    if (token.empty())
        token = std::string(vec::bearerToken(request->getHeader("authorization")).value_or(""));
    const auto userId = tokens_.verify(token);
    if (!userId) return connection->shutdown(closeUnauthorized, "unauthorized");

    auto session = std::make_shared<Session>();
    session->id = nextSessionId++;
    session->userId = *userId;
    session->documentId = request->path().substr(pathPrefix.size());
    connection->setContext(session);

    drogon::async_run([database = database_, rooms = rooms_, connection, session]() -> drogon::Task<> {
        const auto role = co_await vec::MemberRepo(database).roleOf(session->documentId, session->userId);
        if (!role) {
            connection->shutdown(closeNotFound, "not_found");
            co_return;
        }
        session->role = *role;
        session->room = rooms->roomFor(session->documentId);
        session->room->loop()->queueInLoop([session, connection] {
            // A weak pointer, so that the room doesn't keep a closed connection alive.
            session->room->join(session->id, {session->userId,
                                              [weak = std::weak_ptr(connection)](const std::string& message) {
                                                  if (const auto open = weak.lock()) open->send(message);
                                              }});
            vec::logInfo("joined room", {},
                         {{"document", session->documentId}, {"members", session->room->memberCount()}});
        });
        session->ready.store(true, std::memory_order_release);
        sendJson(connection, {{"type", "ready"}, {"role", vec::toString(*role)}});
    });
}

void DocSocket::handleNewMessage(const WebSocketConnectionPtr& connection, std::string&& message,
                                 const drogon::WebSocketMessageType& type) {
    if (type != drogon::WebSocketMessageType::Text) return;  // pings are answered by Drogon itself
    const auto session = connection->getContext<Session>();
    if (!session || !session->ready.load(std::memory_order_acquire)) {
        return sendJson(connection, {{"type", "error"}, {"code", "not_ready"}});
    }
    connection->send(message);
}

void DocSocket::handleConnectionClosed(const WebSocketConnectionPtr& connection) {
    const auto session = connection->getContext<Session>();
    if (!session || !session->ready.load(std::memory_order_acquire)) return;
    session->room->loop()->queueInLoop([session] {
        session->room->leave(session->id);
        vec::logInfo("left room", {},
                     {{"document", session->documentId}, {"members", session->room->memberCount()}});
    });
}
