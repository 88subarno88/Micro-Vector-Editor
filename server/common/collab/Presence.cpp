#include "collab/Presence.h"

#include <algorithm>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace vec {

std::string encodePresence(const PresenceEntry& entry) {
    json out = {{"userId", entry.userId}, {"name", entry.name}, {"seenMs", entry.seenMs}};
    out["cursor"] = entry.cursor ? json{{"x", entry.cursor->x}, {"y", entry.cursor->y}} : json(nullptr);
    return out.dump(-1, ' ', false, json::error_handler_t::replace);
}

std::optional<PresenceEntry> decodePresence(std::string_view text) {
    const json in = json::parse(text, nullptr, false);
    if (!in.is_object() || !in.contains("userId") || !in["userId"].is_string() || !in.contains("seenMs") ||
        !in["seenMs"].is_number_integer()) {
        return std::nullopt;
    }
    PresenceEntry entry{in["userId"], in.value("name", ""), std::nullopt, in["seenMs"]};
    if (const auto cursor = in.find("cursor"); cursor != in.end() && cursor->is_object()) {
        const auto x = cursor->find("x"), y = cursor->find("y");
        if (x != cursor->end() && y != cursor->end() && x->is_number() && y->is_number()) {
            entry.cursor = Point{x->get<double>(), y->get<double>()};
        }
    }
    return entry;
}

std::string presenceMessage(const std::vector<std::string>& values, int64_t nowMs) {
    const int64_t staleMs = std::chrono::milliseconds(presenceStaleAfter).count();
    std::vector<PresenceEntry> entries;
    for (const std::string& value : values) {
        auto entry = decodePresence(value);
        if (entry && nowMs - entry->seenMs <= staleMs) entries.push_back(std::move(*entry));
    }
    std::sort(entries.begin(), entries.end(), [](const PresenceEntry& a, const PresenceEntry& b) {
        return std::tie(a.name, a.userId) < std::tie(b.name, b.userId);
    });

    json users = json::array();
    for (const PresenceEntry& entry : entries) {
        users.push_back({{"id", entry.userId},
                         {"name", entry.name},
                         {"cursor", entry.cursor ? json{{"x", entry.cursor->x}, {"y", entry.cursor->y}}
                                                 : json(nullptr)}});
    }
    return json{{"type", "presence"}, {"users", users}}.dump(-1, ' ', false, json::error_handler_t::replace);
}

int64_t unixMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace vec
