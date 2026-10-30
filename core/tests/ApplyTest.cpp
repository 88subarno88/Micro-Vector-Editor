#include <gtest/gtest.h>

#include "core/Circle.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"
#include "ops/Apply.h"

using namespace vec;

namespace {

const Style redOnBlue{"red", "blue", 3};

// Adds a shape with `geometry` as "a" and checks it saves the same as `expected`.
template <typename Geometry>
void expectAddsLike(GraphicsObject& expected, const Geometry& geometry) {
    expected.setId("a");
    expected.setStrokeColor(redOnBlue.stroke);
    expected.setFillColor(redOnBlue.fill);
    expected.setStrokeWidth(redOnBlue.strokeWidth);

    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {geometry, redOnBlue}}));
    ASSERT_NE(diagram.find("a"), nullptr);
    EXPECT_EQ(diagram.find("a")->getType(), expected.getType());
    EXPECT_EQ(diagram.find("a")->toSVG(), expected.toSVG());
}

}  // namespace

TEST(ApplyTest, AddsARectangle) {
    Rectangle expected(1, 2, 3, 4);
    expectAddsLike(expected, RectGeom{1, 2, 3, 4});
}

TEST(ApplyTest, AddsARoundedRectangle) {
    RoundedRectangle expected(1, 2, 3, 4, 5);
    expectAddsLike(expected, RectGeom{1, 2, 3, 4, 5});
}

TEST(ApplyTest, AddsACircle) {
    Circle expected(1, 2, 3);
    expectAddsLike(expected, CircleGeom{{1, 2}, 3});
}

TEST(ApplyTest, AddsALine) {
    Line expected(1, 2, 3, 4);
    expectAddsLike(expected, LineGeom{{1, 2}, {3, 4}});
}

TEST(ApplyTest, AddsAHexagon) {
    Hexagon expected(1, 2, 3);
    expectAddsLike(expected, HexagonGeom{{1, 2}, 3});
}

TEST(ApplyTest, AddsAFreehandSketch) {
    FreehandSketch expected;
    expected.addPoint(1, 2);
    expected.addPoint(3, 4);
    expectAddsLike(expected, FreehandGeom{{{1, 2}, {3, 4}}});
}

TEST(ApplyTest, AddsText) {
    Text expected(1, 2, "Hi");
    expected.setFrontSize(30);
    expectAddsLike(expected, TextData{{1, 2}, "Hi", 30});
}

TEST(ApplyTest, AddWithATakenIdFailsAndChangesNothing) {
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {CircleGeom{{1, 2}, 3}, {}}}));

    const auto result = apply(diagram, AddShape{"a", {RectGeom{}, {}}});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::DuplicateId);
    EXPECT_EQ(diagram.getObjectCount(), 1);
    EXPECT_EQ(diagram.find("a")->getType(), Circle().getType());
}

TEST(ApplyTest, AddTextWithABadFontSizeFailsInsteadOfThrowing) {
    Diagram diagram;

    const auto result = apply(diagram, AddShape{"a", {TextData{{1, 2}, "Hi", 0}, {}}});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::InvalidShape);
    EXPECT_EQ(diagram.getObjectCount(), 0);
}

TEST(ApplyTest, DeleteRemovesOnlyThatShape) {
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {RectGeom{}, {}}}));
    ASSERT_TRUE(apply(diagram, AddShape{"b", {RectGeom{}, {}}}));

    ASSERT_TRUE(apply(diagram, DeleteShape{"a"}));

    EXPECT_EQ(diagram.find("a"), nullptr);
    EXPECT_NE(diagram.find("b"), nullptr);
}

TEST(ApplyTest, DeleteOfAnUnknownIdFailsAndChangesNothing) {
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {RectGeom{}, {}}}));

    const auto result = apply(diagram, DeleteShape{"nope"});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::ShapeNotFound);
    EXPECT_EQ(diagram.getObjectCount(), 1);
}
