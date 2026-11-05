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

TEST(MessagesTest, AnOpDecodes) {
    const auto message = parseClientMessage(
        R"({"type":"op","clientOpId":"c1-42","baseSeq":118,"op":{"kind":"move","id":"s_9","dx":4,"dy":-2}})");

    ASSERT_TRUE(message) << message.error().message;
    const auto& op = std::get<OpMessage>(*message);
    EXPECT_EQ(op.clientOpId, "c1-42");
    EXPECT_EQ(op.baseSeq, 118);
    EXPECT_DOUBLE_EQ(std::get<Move>(op.op).dy, -2);
}

TEST(MessagesTest, AnOpWithABadClientOpIdOrBaseSeqIsBadMessage) {
    for (const char* text : {
             R"({"type":"op","baseSeq":1,"op":{"kind":"deleteShape","id":"a"}})",
             R"({"type":"op","clientOpId":"","baseSeq":1,"op":{"kind":"deleteShape","id":"a"}})",
             R"({"type":"op","clientOpId":"has space","baseSeq":1,"op":{"kind":"deleteShape","id":"a"}})",
             R"({"type":"op","clientOpId":7,"baseSeq":1,"op":{"kind":"deleteShape","id":"a"}})",
             R"({"type":"op","clientOpId":"c1","op":{"kind":"deleteShape","id":"a"}})",
             R"({"type":"op","clientOpId":"c1","baseSeq":-1,"op":{"kind":"deleteShape","id":"a"}})",
         }) {
        const auto message = parseClientMessage(text);
        ASSERT_FALSE(message) << text;
        EXPECT_EQ(message.error().code, "bad_message") << text;
    }
    const std::string longId(65, 'a');
    EXPECT_FALSE(parseClientMessage(R"({"type":"op","clientOpId":")" + longId +
                                    R"(","baseSeq":1,"op":{"kind":"deleteShape","id":"a"}})"));
}

TEST(MessagesTest, AnUndecodableOpIsBadOpAndNamesTheOp) {
    for (const char* text :
         {R"({"type":"op","clientOpId":"c1","baseSeq":1})",
          R"({"type":"op","clientOpId":"c1","baseSeq":1,"op":{"kind":"explode","id":"a"}})",
          R"({"type":"op","clientOpId":"c1","baseSeq":1,"op":{"kind":"move","id":"a","dx":"far"}})",
          R"({"type":"op","clientOpId":"c1","baseSeq":1,"op":7})"}) {
        const auto message = parseClientMessage(text);
        ASSERT_FALSE(message) << text;
        EXPECT_EQ(message.error().code, "bad_op") << text;
        EXPECT_EQ(message.error().clientOpId, "c1") << text;
        EXPECT_FALSE(message.error().message.empty()) << text;
    }
}

TEST(MessagesTest, ClientOpIds) {
    EXPECT_TRUE(isValidClientOpId("c1-42"));
    EXPECT_TRUE(isValidClientOpId("tab:3.op_9"));
    EXPECT_TRUE(isValidClientOpId(std::string(64, 'a')));
    for (const char* id : {"", "a b", "a/b", "a\"b", "caf\xc3\xa9"})
        EXPECT_FALSE(isValidClientOpId(id)) << id;
}

TEST(MessagesTest, Cursor) {
    const auto message = parseClientMessage(R"({"type":"cursor","x":310,"y":-14.5})");

    ASSERT_TRUE(message);
    EXPECT_DOUBLE_EQ(std::get<CursorMessage>(*message).x, 310);
    EXPECT_DOUBLE_EQ(std::get<CursorMessage>(*message).y, -14.5);
}

TEST(MessagesTest, ABadCursorIsBadMessage) {
    for (const char* text :
         {R"({"type":"cursor"})", R"({"type":"cursor","x":1})", R"({"type":"cursor","x":"1","y":2})",
          R"({"type":"cursor","x":1e308,"y":2})", R"({"type":"cursor","x":null,"y":2})"}) {
        const auto message = parseClientMessage(text);
        ASSERT_FALSE(message) << text;
        EXPECT_EQ(message.error().code, "bad_message") << text;
    }
}
