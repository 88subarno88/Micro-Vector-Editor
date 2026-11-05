#ifndef SERVER_PRESENCE_H
#define SERVER_PRESENCE_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Geometry.h"

namespace vec {

// Who is looking at a document, kept in Redis so that every server instance sees everyone: one hash per
// document, presence:{id}, with a field per connection. Redis 7.0 can only expire a whole hash, so each
// entry also carries when it was last refreshed, and entries older than this are ignored: that covers
// connections lost when an instance crashed, which nobody removes.
constexpr std::chrono::seconds presenceStaleAfter{60};

struct PresenceEntry {
    std::string userId;
    std::string name;
    std::optional<Point> cursor;  // unset until the user moves their pointer
    int64_t seenMs = 0;           // Unix time in milliseconds, comparable across instances
};

std::string encodePresence(const PresenceEntry& entry);
std::optional<PresenceEntry> decodePresence(std::string_view text);

// {"type": "presence", "users": [{"id", "name", "cursor": {"x", "y"} or null}]} from the hash's values,
// leaving out stale and malformed entries. A user with several connections appears once per connection,
// each with its own cursor. Sorted by name, then id, so that the message doesn't change order needlessly.
std::string presenceMessage(const std::vector<std::string>& values, int64_t nowMs);

int64_t unixMillis();

}  // namespace vec

#endif  // SERVER_PRESENCE_H
