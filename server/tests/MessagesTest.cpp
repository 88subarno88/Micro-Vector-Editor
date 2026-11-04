#include <gtest/gtest.h>

#include <string>

#include "collab/Messages.h"

using namespace vec;

TEST(MessagesTest, Ping) {
    EXPECT_TRUE(std::holds_alternative<PingMessage>(*parseClientMessage(R"({"type":"ping"})")));
}

TEST(MessagesTest, JoinWithAndWithoutLastSeq) {
    EXPECT_EQ(std::get<JoinMessage>(*parseClientMessage(R"({"type":"join","lastSeq":118})")).lastSeq, 118);
    EXPECT_EQ(std::get<JoinMessage>(*parseClientMessage(R"({"type":"join"})")).lastSeq, 0);
    EXPECT_EQ(std::get<JoinMessage>(*parseClientMessage(R"({"type":"join","lastSeq":9223372036854775807})"))
                  .lastSeq,
              INT64_MAX);
}

TEST(MessagesTest, FieldsOutsideTheFormatAreIgnored) {
    EXPECT_TRUE(parseClientMessage(R"({"type":"ping","extra":1})"));
}

TEST(MessagesTest, MalformedMessagesAreBadMessage) {
    for (const char* text : {"", "not json", "[]", "42", "null", R"({})", R"({"type":7})", R"({"type":null})",
                             R"({"type":"join","lastSeq":-1})", R"({"type":"join","lastSeq":1.5})",
                             R"({"type":"join","lastSeq":"3"})", R"({"type":"join","lastSeq":true})",
                             R"({"type":"join","lastSeq":9223372036854775808})", R"({"type":"ping")"}) {
        const auto message = parseClientMessage(text);
        ASSERT_FALSE(message) << text;
        EXPECT_EQ(message.error().code, "bad_message") << text;
    }
}

TEST(MessagesTest, AnUnknownTypeIsReportedAsSuch) {
    const auto message = parseClientMessage(R"({"type":"explode"})");

    ASSERT_FALSE(message);
    EXPECT_EQ(message.error().code, "unknown_type");
}
