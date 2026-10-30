#include <gtest/gtest.h>

#include <memory>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Circle.h"
#include "core/Diagram.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"
#include "parser/SVGParser.h"

TEST(ShapeIdTest, HasPrefixAndSixteenHexDigits) {
    EXPECT_TRUE(std::regex_match(GraphicsObject::generateId(), std::regex("s_[0-9a-f]{16}")));
}

TEST(ShapeIdTest, GeneratedIdsAreUnique) {
    std::set<ShapeId> ids;
    for (int i = 0; i < 10000; ++i) ids.insert(GraphicsObject::generateId());

    EXPECT_EQ(ids.size(), 10000u);
}

TEST(ShapeIdTest, IdStaysTheSameAfterEdits) {
    Rectangle shape(0, 0, 10, 10);
    const ShapeId id = shape.getId();

    shape.move(5, 5);
    shape.scale_factor(2);
    shape.setFillColor("red");

    EXPECT_EQ(shape.getId(), id);
}

namespace {

std::vector<std::unique_ptr<GraphicsObject>> oneOfEachShape() {
    auto sketch = std::make_unique<FreehandSketch>();
    sketch->addPoint(1, 2);

    std::vector<std::unique_ptr<GraphicsObject>> shapes;
    shapes.push_back(std::make_unique<Rectangle>());
    shapes.push_back(std::make_unique<RoundedRectangle>(0, 0, 10, 10, 2));
    shapes.push_back(std::make_unique<Circle>());
    shapes.push_back(std::make_unique<Line>());
    shapes.push_back(std::make_unique<Hexagon>());
    shapes.push_back(std::move(sketch));
    shapes.push_back(std::make_unique<Text>(0, 0, "Hi"));
    return shapes;
}

// The id of the only shape in `svg`.
ShapeId loadedId(std::string_view svg) {
    const auto diagram = SVGParser::parse_SVG(svg);
    EXPECT_TRUE(diagram);
    EXPECT_EQ(diagram->getObjectCount(), 1);
    return diagram->getShapes().front()->getId();
}

const std::regex generatedId("s_[0-9a-f]{16}");

}  // namespace

TEST(ShapeIdTest, CloneOfEveryShapeGetsANewId) {
    for (const auto& shape : oneOfEachShape()) {
        EXPECT_NE(shape->clone()->getId(), shape->getId()) << shape->getType();
    }
}

TEST(ShapeIdTest, EveryShapeKeepsItsIdThroughSaveAndLoad) {
    Diagram original;
    std::vector<ShapeId> ids;
    for (auto& shape : oneOfEachShape()) {
        ids.push_back(shape->getId());
        original.addObject(std::move(shape));
    }

    const auto loaded = SVGParser::parse_SVG(original.toSVG());

    ASSERT_TRUE(loaded);
    ASSERT_EQ(loaded->getObjectCount(), ids.size());
    for (size_t i = 0; i < ids.size(); ++i) {
        EXPECT_EQ(loaded->getShapes()[i]->getId(), ids[i]) << loaded->getShapes()[i]->getType();
        EXPECT_NE(loaded->find(ids[i]), nullptr);
    }
}

TEST(ShapeIdTest, KeepsAnIdFromAnotherEditor) {
    EXPECT_EQ(loadedId(R"(<rect id="rect42"/>)"), "rect42");
}

TEST(ShapeIdTest, GeneratesAnIdWhenMissingOrEmpty) {
    EXPECT_TRUE(std::regex_match(loadedId("<rect/>"), generatedId));
    EXPECT_TRUE(std::regex_match(loadedId(R"(<rect id=""/>)"), generatedId));
}

TEST(ShapeIdTest, ReplacesAnIdThatWouldBreakTheSavedFile) {
    EXPECT_TRUE(std::regex_match(loadedId(R"(<rect id='a"b'/>)"), generatedId));
}

TEST(ShapeIdTest, DuplicateIdInAFileGetsANewId) {
    const auto loaded = SVGParser::parse_SVG(R"(<rect id="same" x="1"/><rect id="same" x="2"/>)");

    ASSERT_TRUE(loaded);
    ASSERT_EQ(loaded->getObjectCount(), 2);
    EXPECT_DOUBLE_EQ(loaded->find("same")->getX(), 1);

    const GraphicsObject& second = *loaded->getShapes()[1];
    EXPECT_TRUE(std::regex_match(second.getId(), generatedId));
    EXPECT_EQ(loaded->find(second.getId()), &second);
}
