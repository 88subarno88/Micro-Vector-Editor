#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "core/Diagram.h"
#include "core/Hexagon.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"
#include "parser/SVGParser.h"

namespace {

// Writes `svg` to a temp file and loads it into `diagram`.
void loadFromString(const std::string& svg, Diagram& diagram) {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    const auto path = std::filesystem::temp_directory_path() /
                      (std::string("vec_parser_") + info->name() + ".svg");
    std::ofstream(path) << svg;
    SVGParser::load_SVG(path.string(), diagram);
    std::filesystem::remove(path);
}

// Returns the diagram's single shape as a Hexagon (throws if it is something else).
const Hexagon& onlyHexagon(const Diagram& diagram) {
    EXPECT_EQ(diagram.getObjectCount(), 1);
    return dynamic_cast<const Hexagon&>(*diagram.getShapes().at(0));
}

}  // namespace

TEST(ParserTest, MissingStrokeWidthUsesDefault) {
    Diagram diagram;
    EXPECT_NO_THROW(loadFromString(R"(<rect x="1" y="2" width="3" height="4"/>)", diagram));

    ASSERT_EQ(diagram.getObjectCount(), 1);
    EXPECT_DOUBLE_EQ(diagram.getShapes()[0]->getStrokeWidth(), 1.0);
}

TEST(ParserTest, XIsNotReadFromRx) {
    Diagram diagram;
    loadFromString(R"(<rect rx="5" x="10" y="20" width="30" height="40" stroke-width="1"/>)", diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    const auto& shape = *diagram.getShapes()[0];
    ASSERT_EQ(shape.getType(), "RoundedRectangle");
    EXPECT_DOUBLE_EQ(shape.getX(), 10.0);
    EXPECT_DOUBLE_EQ(static_cast<const RoundedRectangle&>(shape).getCornerRadius(), 5.0);
}

TEST(ParserTest, WidthIsNotReadFromStrokeWidth) {
    Diagram diagram;
    loadFromString(R"(<rect stroke-width="3" x="0" y="0" width="40" height="50"/>)", diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    const auto& shape = *diagram.getShapes()[0];
    EXPECT_DOUBLE_EQ(shape.getWidth(), 40.0);
    EXPECT_DOUBLE_EQ(shape.getStrokeWidth(), 3.0);
}

TEST(ParserTest, ReadsIndentedTags) {
    Diagram diagram;
    loadFromString("<svg>\n    <circle cx=\"1\" cy=\"2\" r=\"3\"/>\n</svg>\n", diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    EXPECT_DOUBLE_EQ(diagram.getShapes()[0]->getWidth(), 6.0);
}

TEST(ParserTest, ReadsTagSpanningMultipleLines) {
    Diagram diagram;
    loadFromString("<rect x=\"10\" y=\"20\"\n"
                   "      width=\"30\" height=\"40\"\n"
                   "      stroke=\"red\"/>\n",
                   diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    const auto& shape = *diagram.getShapes()[0];
    EXPECT_DOUBLE_EQ(shape.getX(), 10.0);
    EXPECT_DOUBLE_EQ(shape.getHeight(), 40.0);
    EXPECT_EQ(shape.getStrokeColor(), "red");
}

TEST(ParserTest, MatchesExactTagNames) {
    Diagram diagram;
    loadFromString(R"(<linearGradient id="g"/><pathology/><line x1="0" y1="0" x2="1" y2="1"/>)",
                   diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    EXPECT_EQ(diagram.getShapes()[0]->getType(), "Line");
}

TEST(ParserTest, ReadsTextContent) {
    Diagram diagram;
    loadFromString("<svg>\n  <text x=\"5\" y=\"6\">Hello world</text>\n</svg>", diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    EXPECT_EQ(diagram.getShapes()[0]->getType(), "Text");
    EXPECT_DOUBLE_EQ(diagram.getShapes()[0]->getX(), 5.0);
}

TEST(ParserTest, OddNumberOfPolygonValuesIgnoresLastOne) {
    Diagram diagram;
    loadFromString(R"(<polygon points="0,0 10,0 5"/>)", diagram);

    const Hexagon& hexagon = onlyHexagon(diagram);
    EXPECT_DOUBLE_EQ(hexagon.getCenterX(), 5.0);
    EXPECT_DOUBLE_EQ(hexagon.getRadius(), 5.0);
}

TEST(ParserTest, PolygonAcceptsSpacesAroundCommas) {
    Diagram diagram;
    loadFromString("<polygon points=\"0 , 0  10 ,0\n 5, 8\"/>", diagram);

    const Hexagon& hexagon = onlyHexagon(diagram);
    EXPECT_DOUBLE_EQ(hexagon.getCenterX(), 5.0);
    EXPECT_DOUBLE_EQ(hexagon.getCenterY(), 4.0);
    EXPECT_DOUBLE_EQ(hexagon.getRadius(), 5.0);
}

TEST(ParserTest, PolygonWithoutPointsAddsNoShape) {
    Diagram diagram;
    loadFromString(R"(<polygon points=""/><polygon points="7"/><polygon/>)", diagram);

    EXPECT_EQ(diagram.getObjectCount(), 0);
}

TEST(ParserTest, TextColorsMapToFillAndStroke) {
    Diagram diagram;
    loadFromString(R"(<text x="0" y="0" fill="red" stroke="blue" stroke-width="2">Hi</text>)",
                   diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    const auto& shape = *diagram.getShapes()[0];
    EXPECT_EQ(shape.getFillColor(), "red");
    EXPECT_EQ(shape.getStrokeColor(), "blue");
    EXPECT_DOUBLE_EQ(shape.getStrokeWidth(), 2.0);
}

TEST(ParserTest, TextColorsSurviveSaveAndLoad) {
    Text original(3, 4, "Hi");
    original.setFillColor("red");
    original.setStrokeColor("blue");
    original.setStrokeWidth(2);

    Diagram diagram;
    loadFromString(original.toSVG(), diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    const auto& loaded = *diagram.getShapes()[0];
    EXPECT_EQ(loaded.getFillColor(), "red");
    EXPECT_EQ(loaded.getStrokeColor(), "blue");
    EXPECT_DOUBLE_EQ(loaded.getStrokeWidth(), 2.0);
}

TEST(ParserTest, MissingColorsKeepDefaults) {
    Diagram diagram;
    loadFromString(R"(<rect x="0" y="0" width="1" height="1"/>)", diagram);

    ASSERT_EQ(diagram.getObjectCount(), 1);
    EXPECT_EQ(diagram.getShapes()[0]->getStrokeColor(), "black");
    EXPECT_EQ(diagram.getShapes()[0]->getFillColor(), "white");
}

TEST(ParserTest, InvalidNumberDoesNotThrow) {
    Diagram diagram;
    EXPECT_NO_THROW(loadFromString(R"(<circle cx="abc" cy="5" r="10" stroke-width="1"/>)", diagram));
}
