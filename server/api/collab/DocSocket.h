#ifndef DOC_SOCKET_H
#define DOC_SOCKET_H

#include <drogon/WebSocketController.h>
#include <drogon/nosql/RedisSubscriber.h>
#include <drogon/orm/DbClient.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "auth/AccessToken.h"
#include "collab/Messages.h"
#include "collab/OpPipeline.h"
#include "collab/Room.h"

struct DocSession;

// The live connection to one document: /ws/docs/{id}?token=<access token>. Browsers can't set headers on
// a WebSocket, hence the query parameter (request logs record the path only, never the query).
//
// The token is checked first, then the user's role on the document. On failure the server closes with
// 4401 (bad or expired token) or 4404 (no such document, or not a member), and at any time with 4408
// after 45 s in which the client sent nothing. On success it sends
// {"type": "ready", "role": ...}; anything the client sends before that gets {"type": "error",
// "code": "not_ready"}. A ready connection is a member of its document's room until it closes; the first
// member loads the room's drawing from the database.
//
// Then the client sends messages (see collab/Messages.h):
//   {"type": "join", "lastSeq": N} -> once the drawing is loaded, {"type": "joined", "headSeq": H} with
//                                     either "ops": [{"seq", "userId", "op"}] for every op after N, or,
//                                     when the room no longer has them all, "snapshot": the whole drawing
//                                     at H. Ops are broadcast to a connection only after this, from H+1.
//                                     {"type": "error", "code": "load_failed"} and close 1011 if the
//                                     drawing can't be loaded
//   {"type": "ping"}               -> {"type": "pong"}. Send one at least every 45 s (15 s is a good
//                                     interval): a connection silent for 45 s is closed.
//   {"type": "op", ...}            -> 30 a second on average, bursts of 60; beyond that, reject with
//                                     reason rate_limited. Otherwise {"type": "reject", "clientOpId",
//                                     "reason", "message"}, reason bad_op for an op that can't be decoded, or
//                                     forbidden for a viewer
// Anything malformed gets {"type": "error", "code": "bad_message" or "unknown_type", "message": ...}.
class DocSocket : public drogon::WebSocketController<DocSocket, false> {
public:
    // A room is unloaded `unloadDelay` after its last member leaves, unless someone joins again first.
    // Ops this instance commits go to `publish`; ops other instances publish arrive on `subscriber`, which
    // listens on each document this instance has a room for.
    DocSocket(vec::AccessTokens tokens, drogon::orm::DbClientPtr database,
              std::shared_ptr<vec::RoomRegistry> rooms, std::chrono::seconds unloadDelay,
              vec::OpPublisher publish, std::shared_ptr<drogon::nosql::RedisSubscriber> subscriber)
        : tokens_(std::move(tokens)),
          database_(std::move(database)),
          rooms_(std::move(rooms)),
          unloadDelay_(unloadDelay),
          publish_(std::move(publish)),
          subscriber_(std::move(subscriber)) {}

    void handleNewConnection(const drogon::HttpRequestPtr& request,
                             const drogon::WebSocketConnectionPtr& connection) override;
    void handleNewMessage(const drogon::WebSocketConnectionPtr& connection, std::string&& message,
                          const drogon::WebSocketMessageType& type) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& connection) override;

    WS_PATH_LIST_BEGIN
    WS_ADD_PATH_VIA_REGEX("/ws/docs/[^/]+");
    WS_PATH_LIST_END

private:
    void joinRoom(const drogon::WebSocketConnectionPtr& connection,
                  const std::shared_ptr<DocSession>& session);
    void loadRoom(const std::shared_ptr<vec::Room>& room);
    void handleJoin(const drogon::WebSocketConnectionPtr& connection,
                    const std::shared_ptr<DocSession>& session, int64_t lastSeq);
    void handleOp(const drogon::WebSocketConnectionPtr& connection,
                  const std::shared_ptr<DocSession>& session, const vec::OpMessage& op);
    void leaveRoom(const std::shared_ptr<DocSession>& session);
    void subscribe(const std::string& documentId);
    void unsubscribeIfUnused(const std::string& documentId);

    vec::AccessTokens tokens_;
    drogon::orm::DbClientPtr database_;
    std::shared_ptr<vec::RoomRegistry> rooms_;
    std::chrono::seconds unloadDelay_;
    vec::OpPublisher publish_;
    std::shared_ptr<drogon::nosql::RedisSubscriber> subscriber_;
    std::mutex subscriptionsMutex_;
    std::unordered_set<std::string> subscriptions_;  // documents whose channel subscriber_ listens on
};

#endif  // DOC_SOCKET_H
