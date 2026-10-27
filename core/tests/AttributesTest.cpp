#include <gtest/gtest.h>

#include "parser/SVGParser.h"

using Attributes = SVGParser::Attributes;

TEST(AttributesTest, ParsesPairs) {
    EXPECT_EQ(SVGParser::parseAttributes(R"(<rect x="1" y="2"/>)"),
              (Attributes{{"x", "1"}, {"y", "2"}}));
}

TEST(AttributesTest, SimilarNamesAreSeparate) {
    const auto attrs =
        SVGParser::parseAttributes(R"(<rect rx="5" x="10" stroke-width="3" width="40">)");

    EXPECT_EQ(attrs.at("x"), "10");
    EXPECT_EQ(attrs.at("rx"), "5");
    EXPECT_EQ(attrs.at("width"), "40");
    EXPECT_EQ(attrs.at("stroke-width"), "3");
}

TEST(AttributesTest, AcceptsSingleQuotesAndSpacesAroundEquals) {
    EXPECT_EQ(SVGParser::parseAttributes("<circle  r = '4'\n\tcx=\"1\" />"),
              (Attributes{{"r", "4"}, {"cx", "1"}}));
}

TEST(AttributesTest, StopsAtEndOfTag) {
    EXPECT_EQ(SVGParser::parseAttributes(R"(<text x="1">a="b"</text>)"),
              (Attributes{{"x", "1"}}));
}

TEST(AttributesTest, ValueMayContainSpecialCharacters) {
    EXPECT_EQ(SVGParser::parseAttributes(R"(<path d="M 1 2 L 3>4"/>)").at("d"), "M 1 2 L 3>4");
}

TEST(ParseNumberTest, ParsesNumbers) {
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("10", 0), 10.0);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("-2.5", 0), -2.5);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber(" +1.5e2", 0), 150.0);
}

TEST(ParseNumberTest, IgnoresUnitSuffix) {
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("10px", 0), 10.0);
}

TEST(ParseNumberTest, ReturnsFallbackForBadInput) {
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("", 7), 7.0);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("abc", 7), 7.0);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("   ", 7), 7.0);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("inf", 7), 7.0);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("nan", 7), 7.0);
    EXPECT_DOUBLE_EQ(SVGParser::parseNumber("1e999", 7), 7.0);
}

TEST(AttributesTest, MalformedInputReturnsPairsSoFar) {
    EXPECT_EQ(SVGParser::parseAttributes(R"(<rect x="1" y="2)"), (Attributes{{"x", "1"}}));
    EXPECT_EQ(SVGParser::parseAttributes(R"(<rect x="1" y 2>)"), (Attributes{{"x", "1"}}));
    EXPECT_TRUE(SVGParser::parseAttributes("").empty());
}
