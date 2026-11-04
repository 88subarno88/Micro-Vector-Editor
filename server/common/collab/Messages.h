#ifndef SERVER_MESSAGES_H
#define SERVER_MESSAGES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <variant>

namespace vec {

// Messages a client sends over a document's WebSocket, as JSON objects with a "type" field.

// {"type": "join", "lastSeq": 118}: start receiving the document. lastSeq is the newest op the client
// already has, 0 for none.
struct JoinMessage {
    int64_t lastSeq = 0;
};

// {"type": "ping"}: a heartbeat; the server answers {"type": "pong"}.
struct PingMessage {};

using ClientMessage = std::variant<JoinMessage, PingMessage>;

struct MessageError {
    std::string code;  // bad_message or unknown_type
    std::string message;
};

// Decodes untrusted input; never throws.
tl::expected<ClientMessage, MessageError> parseClientMessage(std::string_view text);

}  // namespace vec

#endif  // SERVER_MESSAGES_H
