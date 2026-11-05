#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "collab/Presence.h"

using namespace vec;
using nlohmann::json;

namespace {

constexpr int64_t now = 1'000'000'000;

std::string entry(const std::string& id, const std::string& name, int64_t seenMs,
                  std::optional<Point> cursor = {}) {
    return encodePresence({id, name, cursor, seenMs});
}

json users(const std::vector<std::string>& values) {
    return json::parse(presenceMessage(values, now)).at("users");
}

}  // namespace

TEST(PresenceTest, EntriesRoundTrip) {
    const auto decoded = decodePresence(encodePresence({"u1", "ann@x.io", Point{3, 4}, now}));

    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->userId, "u1");
    EXPECT_EQ(decoded->name, "ann@x.io");
    EXPECT_DOUBLE_EQ(decoded->cursor->x, 3);
    EXPECT_EQ(decoded->seenMs, now);
    EXPECT_FALSE(decodePresence(encodePresence({"u1", "ann", std::nullopt, now}))->cursor);
}

TEST(PresenceTest, TheMessageListsEveryoneWithTheirCursor) {
    const json list = users({entry("u2", "bob", now, Point{1, 2}), entry("u1", "ann", now)});

    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0], json({{"id", "u1"}, {"name", "ann"}, {"cursor", nullptr}}));
    EXPECT_EQ(list[1], json({{"id", "u2"}, {"name", "bob"}, {"cursor", {{"x", 1.0}, {"y", 2.0}}}}));
}

TEST(PresenceTest, StaleEntriesAreLeftOut) {
    const int64_t limit = std::chrono::milliseconds(presenceStaleAfter).count();

    const json list = users({entry("fresh", "a", now - limit), entry("stale", "b", now - limit - 1)});

    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].at("id"), "fresh");
}

TEST(PresenceTest, MalformedEntriesAreLeftOut) {
    EXPECT_TRUE(
        users({"", "not json", "[]", R"({"name":"x"})", R"({"userId":"u","seenMs":"soon"})"}).empty());
}

TEST(PresenceTest, ASecondConnectionOfOneUserIsListedSeparately) {
    EXPECT_EQ(users({entry("u1", "ann", now, Point{1, 1}), entry("u1", "ann", now, Point{9, 9})}).size(), 2u);
}

TEST(PresenceTest, AnEmptyRoomHasNoUsers) {
    EXPECT_EQ(json::parse(presenceMessage({}, now)), json({{"type", "presence"}, {"users", json::array()}}));
}
