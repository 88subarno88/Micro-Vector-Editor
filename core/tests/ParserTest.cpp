#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "core/Diagram.h"
#include "core/RoundedRectangle.h"
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

TEST(ParserTest, InvalidNumberDoesNotThrow) {
    Diagram diagram;
    EXPECT_NO_THROW(loadFromString(R"(<circle cx="abc" cy="5" r="10" stroke-width="1"/>)", diagram));
}
