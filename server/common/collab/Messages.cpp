#include "collab/Messages.h"

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

#include "ops/Json.h"

using nlohmann::json;

namespace vec {

namespace {

// The non-negative integer at `key`; MessageError if it's missing or anything else.
tl::expected<int64_t, MessageError> seqField(const json& message, const char* key, bool required) {
    const auto it = message.find(key);
    if (it == message.end()) {
        if (required)
            return tl::unexpected(MessageError{"bad_message", std::string(key) + " is required", {}});
        return 0;
    }
    if (!it->is_number_unsigned() || it->get<uint64_t>() > static_cast<uint64_t>(INT64_MAX)) {
        return tl::unexpected(
            MessageError{"bad_message", std::string(key) + " must be a non-negative integer", {}});
    }
    return static_cast<int64_t>(it->get<uint64_t>());
}

tl::expected<ClientMessage, MessageError> parseOp(const json& message) {
    const auto id = message.find("clientOpId");
    if (id == message.end() || !id->is_string() || !isValidClientOpId(id->get_ref<const std::string&>())) {
        return tl::unexpected(MessageError{
            "bad_message", "clientOpId must be 1 to 64 letters, digits, '.', '_', ':' or '-'", {}});
    }
    const std::string clientOpId = id->get<std::string>();

    const auto baseSeq = seqField(message, "baseSeq", true);
    if (!baseSeq) return tl::unexpected(baseSeq.error());

    const auto op = message.find("op");
    if (op == message.end()) return tl::unexpected(MessageError{"bad_op", "op is required", clientOpId});
    auto decoded = decodeOperation(*op);
    if (!decoded) return tl::unexpected(MessageError{"bad_op", decoded.error().message, clientOpId});
    return OpMessage{clientOpId, *baseSeq, std::move(*decoded)};
}

}  // namespace

bool isValidClientOpId(std::string_view id) {
    if (id.empty() || id.size() > 64) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::isalnum(c) != 0 || c == '.' || c == '_' || c == ':' || c == '-';
    });
}

tl::expected<ClientMessage, MessageError> parseClientMessage(std::string_view text) {
    const json message = json::parse(text, nullptr, false);
    if (!message.is_object())
        return tl::unexpected(MessageError{"bad_message", "expected a JSON object", {}});
    const auto type = message.find("type");
    if (type == message.end() || !type->is_string()) {
        return tl::unexpected(MessageError{"bad_message", "expected a string field type", {}});
    }

    if (*type == "ping") return PingMessage{};
    if (*type == "join") {
        const auto lastSeq = seqField(message, "lastSeq", false);
        if (!lastSeq) return tl::unexpected(lastSeq.error());
        return JoinMessage{*lastSeq};
    }
    if (*type == "op") return parseOp(message);
    return tl::unexpected(MessageError{"unknown_type", "unknown message type", {}});
}

}  // namespace vec
