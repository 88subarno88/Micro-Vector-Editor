#include <gtest/gtest.h>

#include <memory>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "core/Circle.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"

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

TEST(ShapeIdTest, CloneOfEveryShapeGetsANewId) {
    std::vector<std::unique_ptr<GraphicsObject>> shapes;
    shapes.push_back(std::make_unique<Rectangle>());
    shapes.push_back(std::make_unique<RoundedRectangle>(0, 0, 10, 10, 2));
    shapes.push_back(std::make_unique<Circle>());
    shapes.push_back(std::make_unique<Line>());
    shapes.push_back(std::make_unique<Hexagon>());
    shapes.push_back(std::make_unique<FreehandSketch>());
    shapes.push_back(std::make_unique<Text>(0, 0, "Hi"));

    for (const auto& shape : shapes) {
        EXPECT_NE(shape->clone()->getId(), shape->getId()) << shape->getType();
    }
}
