#include <gtest/gtest.h>

#include <string>

#include "logging/Log.h"

using namespace vec;
using nlohmann::json;

TEST(LogTest, LineHasTimeLevelAndMessage) {
    const json line = json::parse(formatLogLine("info", "hello"));

    EXPECT_EQ(line.at("level"), "info");
    EXPECT_EQ(line.at("msg"), "hello");
    EXPECT_FALSE(line.contains("request_id"));
    const std::string time = line.at("time");
    EXPECT_EQ(time.size(), 24u) << time;
    EXPECT_EQ(time.back(), 'Z');
}

TEST(LogTest, LineIncludesRequestIdAndFields) {
    const json line = json::parse(formatLogLine("error", "failed", "abc", {{"status", 500}}));

    EXPECT_EQ(line.at("request_id"), "abc");
    EXPECT_EQ(line.at("status"), 500);
}

TEST(LogTest, QuotesAndNewlinesStayOnOneValidLine) {
    const std::string text = formatLogLine("info", "a\"b\nc\",\"level\":\"forged");

    EXPECT_EQ(text.find('\n'), std::string::npos);
    const json line = json::parse(text);
    EXPECT_EQ(line.at("msg"), "a\"b\nc\",\"level\":\"forged");
    EXPECT_EQ(line.at("level"), "info");
}

TEST(LogTest, InvalidUtf8StillGivesValidJson) {
    EXPECT_TRUE(json::accept(formatLogLine("info", "caf\xe9")));
    EXPECT_TRUE(json::accept(formatLogLine("info", "m", "", {{"path", std::string("/\xff")}})));
}

TEST(LogTest, AcceptsSafeRequestIds) {
    for (const char* id : {"a", "abc-123", "A.b_C-9", "550e8400-e29b-41d4-a716-446655440000"}) {
        EXPECT_TRUE(isValidRequestId(id)) << id;
    }
    EXPECT_TRUE(isValidRequestId(std::string(64, 'a')));
}

TEST(LogTest, RejectsUnsafeRequestIds) {
    for (const char* id : {"", "a b", "a\nb", "a\r\nSet-Cookie: x=1", "a\"b", "a/b", "caf\xc3\xa9"}) {
        EXPECT_FALSE(isValidRequestId(id)) << id;
    }
    EXPECT_FALSE(isValidRequestId(std::string(65, 'a')));
}
