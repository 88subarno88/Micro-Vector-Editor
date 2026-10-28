#include <gtest/gtest.h>

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>

#include "parser/SVGParser.h"

namespace {

struct Fixture {
    std::string file;
    int shapes;  // expected number of shapes after loading
};

// Lets GoogleTest print the file name instead of raw bytes.
void PrintTo(const Fixture& fixture, std::ostream* os) { *os << fixture.file; }

std::string fixturePath(const std::string& file) {
    return std::string(FIXTURE_DIR) + "/" + file;
}

// Rounds every number in `svg` to 6 significant digits. A polygon's centre is rebuilt from
// its rounded vertices, so a save/load may change the last digits without moving anything.
std::string roundNumbers(const std::string& svg) {
    static const std::regex number(R"(\d+(\.\d+)?)");
    std::ostringstream out;
    out << std::setprecision(6);
    auto last = svg.cbegin();
    for (std::sregex_iterator it(svg.begin(), svg.end(), number), end; it != end; ++it) {
        out << std::string(last, (*it)[0].first) << std::stod(it->str());
        last = (*it)[0].second;
    }
    out << std::string(last, svg.cend());
    return out.str();
}

class FixtureTest : public ::testing::TestWithParam<Fixture> {};

}  // namespace

TEST_P(FixtureTest, LoadsWithExpectedShapeCount) {
    const auto result = SVGParser::load_SVG(fixturePath(GetParam().file));

    ASSERT_TRUE(result) << "line " << result.error().line << ": " << result.error().message;
    EXPECT_EQ(result->getObjectCount(), GetParam().shapes);
}

TEST_P(FixtureTest, SurvivesSaveAndLoad) {
    const auto loaded = SVGParser::load_SVG(fixturePath(GetParam().file));
    ASSERT_TRUE(loaded);

    const std::string saved = loaded->toSVG();
    const auto reloaded = SVGParser::parse_SVG(saved);
    ASSERT_TRUE(reloaded) << reloaded.error().message;
    EXPECT_EQ(roundNumbers(reloaded->toSVG()), roundNumbers(saved));
}

INSTANTIATE_TEST_SUITE_P(Fixtures, FixtureTest,
                         ::testing::Values(Fixture{"app-export.svg", 7}, Fixture{"rect.svg", 2},
                                           Fixture{"circle.svg", 1}, Fixture{"line.svg", 2},
                                           Fixture{"text.svg", 2}, Fixture{"polygon.svg", 2},
                                           Fixture{"path.svg", 1}),
                         [](const auto& info) {  // "app-export.svg" -> "app_export"
                             std::string name = info.param.file.substr(0, info.param.file.find('.'));
                             std::replace(name.begin(), name.end(), '-', '_');
                             return name;
                         });

TEST(FixtureTest, MalformedFileReportsWhere) {
    const auto result = SVGParser::load_SVG(fixturePath("malformed.svg"));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().line, 6);
    EXPECT_EQ(result.error().message, "unclosed tag");
}
