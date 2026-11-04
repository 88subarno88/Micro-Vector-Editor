#ifndef DOC_SOCKET_H
#define DOC_SOCKET_H

#include <drogon/WebSocketController.h>
#include <drogon/orm/DbClient.h>

#include "auth/AccessToken.h"
#include "collab/Room.h"

// The live connection to one document: /ws/docs/{id}?token=<access token>. Browsers can't set headers on
// a WebSocket, hence the query parameter (request logs record the path only, never the query).
//
// The token is checked first, then the user's role on the document. On failure the server closes with
// 4401 (bad or expired token) or 4404 (no such document, or not a member). On success it sends
// {"type": "ready", "role": ...}; anything the client sends before that gets {"type": "error",
// "code": "not_ready"}. A ready connection is a member of its document's room until it closes. For now
// every later message is echoed back.
class DocSocket : public drogon::WebSocketController<DocSocket, false> {
public:
    DocSocket(vec::AccessTokens tokens, drogon::orm::DbClientPtr database,
              std::shared_ptr<vec::RoomRegistry> rooms)
        : tokens_(std::move(tokens)), database_(std::move(database)), rooms_(std::move(rooms)) {}

    void handleNewConnection(const drogon::HttpRequestPtr& request,
                             const drogon::WebSocketConnectionPtr& connection) override;
    void handleNewMessage(const drogon::WebSocketConnectionPtr& connection, std::string&& message,
                          const drogon::WebSocketMessageType& type) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& connection) override;

    WS_PATH_LIST_BEGIN
    WS_ADD_PATH_VIA_REGEX("/ws/docs/[^/]+");
    WS_PATH_LIST_END

private:
    vec::AccessTokens tokens_;
    drogon::orm::DbClientPtr database_;
    std::shared_ptr<vec::RoomRegistry> rooms_;
};

#endif  // DOC_SOCKET_H
