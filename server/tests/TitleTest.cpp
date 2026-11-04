#include <gtest/gtest.h>

#include <string>

#include "docs/Title.h"

using namespace vec;

TEST(TitleTest, TrimsSpaces) { EXPECT_EQ(normalizeTitle("  My drawing  "), "My drawing"); }

TEST(TitleTest, KeepsUnicode) {
    EXPECT_EQ(normalizeTitle("Caf\xc3\xa9 \xf0\x9f\x8e\xa8"), "Caf\xc3\xa9 \xf0\x9f\x8e\xa8");
}

TEST(TitleTest, RejectsEmptyAndBlank) {
    EXPECT_EQ(normalizeTitle(""), std::nullopt);
    EXPECT_EQ(normalizeTitle("    "), std::nullopt);
}

TEST(TitleTest, RejectsControlCharacters) {
    for (const std::string& title : {std::string("a\nb"), std::string("a\tb"), std::string("a\0b", 3),
                                     std::string("a\x7f"), std::string("\x1b[31mred")}) {
        EXPECT_EQ(normalizeTitle(title), std::nullopt) << title.size();
    }
}

TEST(TitleTest, LengthIsCappedAfterTrimming) {
    EXPECT_TRUE(normalizeTitle(std::string(200, 'x')));
    EXPECT_FALSE(normalizeTitle(std::string(201, 'x')));
    EXPECT_TRUE(normalizeTitle("  " + std::string(200, 'x') + "  "));
}
