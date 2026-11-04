#ifndef SERVER_MESSAGES_H
#define SERVER_MESSAGES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <variant>

#include "ops/Operation.h"

namespace vec {

// Messages a client sends over a document's WebSocket, as JSON objects with a "type" field.

// {"type": "join", "lastSeq": 118}: start receiving the document. lastSeq is the newest op the client
// already has, 0 for none.
struct JoinMessage {
    int64_t lastSeq = 0;
};

// {"type": "ping"}: a heartbeat; the server answers {"type": "pong"}.
struct PingMessage {};

// {"type": "op", "clientOpId": "c1-42", "baseSeq": 118, "op": {...}}: an edit. clientOpId is the
// client's own ID for it, unique for the document, so that a resend can't apply twice; baseSeq is the
// newest op the client had applied when it made this one.
struct OpMessage {
    std::string clientOpId;
    int64_t baseSeq = 0;
    Operation op;
};

using ClientMessage = std::variant<JoinMessage, PingMessage, OpMessage>;

struct MessageError {
    std::string code;  // bad_message, unknown_type, or bad_op
    std::string message;
    std::string clientOpId;  // for bad_op, so that the reply can say which op was rejected
};

// A client op ID may be 1 to 64 letters, digits, '.', '_', ':' or '-'.
bool isValidClientOpId(std::string_view id);

// Decodes untrusted input; never throws.
tl::expected<ClientMessage, MessageError> parseClientMessage(std::string_view text);

}  // namespace vec

#endif  // SERVER_MESSAGES_H
