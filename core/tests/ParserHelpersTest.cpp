#include <gtest/gtest.h>

#include <string_view>
#include <vector>

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

TEST(AttributesTest, MalformedInputReturnsPairsSoFar) {
    EXPECT_EQ(SVGParser::parseAttributes(R"(<rect x="1" y="2)"), (Attributes{{"x", "1"}}));
    EXPECT_EQ(SVGParser::parseAttributes(R"(<rect x="1" y 2>)"), (Attributes{{"x", "1"}}));
    EXPECT_TRUE(SVGParser::parseAttributes("").empty());
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

TEST(DecodeEntitiesTest, DecodesTheFiveXmlEntities) {
    EXPECT_EQ(SVGParser::decodeEntities("&lt;a&gt; &amp; &quot;b&quot; &apos;c&apos;"),
              "<a> & \"b\" 'c'");
}

TEST(DecodeEntitiesTest, DecodesOnlyOnce) {
    EXPECT_EQ(SVGParser::decodeEntities("&amp;lt;"), "&lt;");
}

TEST(DecodeEntitiesTest, LeavesUnknownOrBrokenEntitiesAsIs) {
    EXPECT_EQ(SVGParser::decodeEntities("a & b &foo; &lt"), "a & b &foo; &lt");
}

namespace {

std::vector<std::string_view> allTags(std::string_view doc) {
    std::vector<std::string_view> tags;
    size_t pos = 0;
    for (auto tag = SVGParser::nextTag(doc, pos); !tag.empty(); tag = SVGParser::nextTag(doc, pos)) {
        tags.push_back(tag);
    }
    return tags;
}

}  // namespace

TEST(NextTagTest, ReturnsTagsInOrder) {
    EXPECT_EQ(allTags(R"(<svg><rect x="1"/></svg>)"),
              (std::vector<std::string_view>{"<svg>", R"(<rect x="1"/>)", "</svg>"}));
}

TEST(NextTagTest, SkipsDeclarationsAndComments) {
    EXPECT_EQ(allTags("<?xml version=\"1.0\"?>\n<!DOCTYPE svg>\n<!-- <rect/> -->\n<circle/>"),
              (std::vector<std::string_view>{"<circle/>"}));
}

TEST(NextTagTest, ReturnsTagSpanningLinesWhole) {
    EXPECT_EQ(allTags("<rect x=\"1\"\n   y=\"2\"/>"),
              (std::vector<std::string_view>{"<rect x=\"1\"\n   y=\"2\"/>"}));
}

TEST(NextTagTest, IgnoresGreaterThanInsideQuotes) {
    EXPECT_EQ(allTags(R"(<path d="L 3>4"/>)"),
              (std::vector<std::string_view>{R"(<path d="L 3>4"/>)"}));
}

TEST(NextTagTest, LeavesPosAfterTagForTextContent) {
    const std::string_view doc = R"(<text x="1">Hi</text>)";
    size_t pos = 0;
    SVGParser::nextTag(doc, pos);
    EXPECT_EQ(doc.substr(pos, 2), "Hi");
}

TEST(NextTagTest, LeavesPosAtUnclosedTag) {
    const std::string_view doc = R"(<svg> <rect x="1)";
    size_t pos = 0;
    SVGParser::nextTag(doc, pos);  // <svg>

    EXPECT_TRUE(SVGParser::nextTag(doc, pos).empty());
    EXPECT_EQ(pos, 6u);
}

TEST(NextTagTest, StopsOnUnclosedInput) {
    EXPECT_TRUE(allTags(R"(<rect x="1)").empty());
    EXPECT_TRUE(allTags("<!-- never closed <rect/>").empty());
    EXPECT_TRUE(allTags("").empty());
}
