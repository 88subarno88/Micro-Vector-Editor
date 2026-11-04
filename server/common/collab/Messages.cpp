#include "collab/Messages.h"

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace vec {

tl::expected<ClientMessage, MessageError> parseClientMessage(std::string_view text) {
    const json message = json::parse(text, nullptr, false);
    if (!message.is_object()) return tl::unexpected(MessageError{"bad_message", "expected a JSON object"});
    const auto type = message.find("type");
    if (type == message.end() || !type->is_string()) {
        return tl::unexpected(MessageError{"bad_message", "expected a string field type"});
    }

    if (*type == "ping") return PingMessage{};
    if (*type == "join") {
        const auto lastSeq = message.find("lastSeq");
        if (lastSeq == message.end()) return JoinMessage{};
        if (!lastSeq->is_number_unsigned()) {
            return tl::unexpected(MessageError{"bad_message", "lastSeq must be a non-negative integer"});
        }
        const uint64_t value = lastSeq->get<uint64_t>();
        if (value > static_cast<uint64_t>(INT64_MAX)) {
            return tl::unexpected(MessageError{"bad_message", "lastSeq is too large"});
        }
        return JoinMessage{static_cast<int64_t>(value)};
    }
    return tl::unexpected(MessageError{"unknown_type", "unknown message type"});
}

}  // namespace vec
