#include <gtest/gtest.h>

#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "core/Circle.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"

namespace {

std::unique_ptr<FreehandSketch> sketch() {
    auto shape = std::make_unique<FreehandSketch>();
    shape->addPoint(10, 20);
    shape->addPoint(30, 60);
    return shape;
}

std::unique_ptr<Text> bigText() {
    auto shape = std::make_unique<Text>(10, 20, "Hi");
    shape->setFrontSize(40);
    return shape;
}

// One shape of every kind, each with a known point inside it.
struct Sample {
    std::unique_ptr<GraphicsObject> shape;
    Point inside;
};

std::vector<Sample> samples() {
    std::vector<Sample> all;
    all.push_back({std::make_unique<Rectangle>(10, 20, 30, 40), {25, 40}});
    all.push_back({std::make_unique<RoundedRectangle>(10, 20, 30, 40, 5), {25, 40}});
    all.push_back({std::make_unique<Circle>(10, 20, 30), {10, 20}});
    all.push_back({std::make_unique<Line>(10, 20, 30, 40), {20, 30}});
    all.push_back({std::make_unique<Hexagon>(10, 20, 30), {10, 20}});
    all.push_back({sketch(), {20, 40}});
    all.push_back({bigText(), {20, 30}});
    return all;
}

std::string withoutId(const GraphicsObject& shape) {
    return std::regex_replace(shape.toSVG(), std::regex(R"(id="[^"]*" )"), "");
}

// Checks the bounding box of `shape`.
void expectBox(const GraphicsObject& shape, double x, double y, double width, double height) {
    EXPECT_DOUBLE_EQ(shape.getX(), x) << shape.getType();
    EXPECT_DOUBLE_EQ(shape.getY(), y) << shape.getType();
    EXPECT_DOUBLE_EQ(shape.getWidth(), width) << shape.getType();
    EXPECT_DOUBLE_EQ(shape.getHeight(), height) << shape.getType();
}

}  // namespace

TEST(ShapeTest, RectangleContainsItsAreaAndEdges) {
    const Rectangle shape(10, 20, 30, 40);
    EXPECT_TRUE(shape.contains(25, 40));
    EXPECT_TRUE(shape.contains(10, 20));
    EXPECT_TRUE(shape.contains(40, 60));
    EXPECT_FALSE(shape.contains(9, 40));
    EXPECT_FALSE(shape.contains(41, 40));
    EXPECT_FALSE(shape.contains(25, 61));
}

TEST(ShapeTest, RoundedRectangleContainsItsWholeBox) {
    const RoundedRectangle shape(10, 20, 30, 40, 5);
    EXPECT_TRUE(shape.contains(25, 40));
    EXPECT_TRUE(shape.contains(10, 20));  // the corner is cut off when drawn, but still counts
    EXPECT_FALSE(shape.contains(9, 40));
}

TEST(ShapeTest, CircleContainsPointsWithinItsRadius) {
    const Circle shape(10, 20, 30);
    EXPECT_TRUE(shape.contains(10, 20));
    EXPECT_TRUE(shape.contains(40, 20));
    EXPECT_FALSE(shape.contains(41, 20));
    EXPECT_FALSE(shape.contains(35, 45));  // inside the bounding box, outside the circle
}

TEST(ShapeTest, LineContainsPointsWithinItsTolerance) {
    const Line shape(0, 0, 100, 0);
    EXPECT_TRUE(shape.contains(50, 0));
    EXPECT_TRUE(shape.contains(50, 25));
    EXPECT_FALSE(shape.contains(50, 26));
    EXPECT_TRUE(shape.contains(125, 0));   // past the end, within tolerance
    EXPECT_FALSE(shape.contains(126, 0));
}

TEST(ShapeTest, ZeroLengthLineContainsPointsNearIt) {
    const Line shape(10, 10, 10, 10);
    EXPECT_TRUE(shape.contains(10, 35));
    EXPECT_FALSE(shape.contains(10, 36));
}

TEST(ShapeTest, HexagonContainsPointsWithinItsRadius) {
    const Hexagon shape(10, 20, 30);
    EXPECT_TRUE(shape.contains(10, 20));
    EXPECT_TRUE(shape.contains(40, 20));
    EXPECT_FALSE(shape.contains(41, 20));
}

TEST(ShapeTest, FreehandSketchContainsItsBoundingBox) {
    const auto shape = sketch();
    EXPECT_TRUE(shape->contains(20, 40));
    EXPECT_FALSE(shape->contains(9, 40));
    EXPECT_FALSE(shape->contains(20, 61));
}

TEST(ShapeTest, EmptyFreehandSketchHasNothingToSave) {
    EXPECT_EQ(FreehandSketch().toSVG(), "");
}

TEST(ShapeTest, TextContainsItsEstimatedBox) {
    const Text shape(10, 20, "Hi");  // 0.6 * 16 * 2 wide, 1.5 * 16 tall
    EXPECT_TRUE(shape.contains(29.2, 44));
    EXPECT_FALSE(shape.contains(29.3, 30));
    EXPECT_FALSE(shape.contains(15, 44.1));
}

TEST(ShapeTest, MoveShiftsEveryShapeAndItsHitArea) {
    for (const Sample& sample : samples()) {
        GraphicsObject& shape = *sample.shape;
        const double x = shape.getX(), y = shape.getY();

        shape.move(1000, 2000);

        EXPECT_DOUBLE_EQ(shape.getX(), x + 1000) << shape.getType();
        EXPECT_DOUBLE_EQ(shape.getY(), y + 2000) << shape.getType();
        EXPECT_TRUE(shape.contains(sample.inside.x + 1000, sample.inside.y + 2000)) << shape.getType();
        EXPECT_FALSE(shape.contains(sample.inside.x, sample.inside.y)) << shape.getType();
    }
}

TEST(ShapeTest, ScaleResizesRectanglesFromTheirCorner) {
    Rectangle rect(10, 20, 30, 40);
    rect.scale_factor(2);
    expectBox(rect, 10, 20, 60, 80);

    RoundedRectangle rounded(10, 20, 30, 40, 5);
    rounded.scale_factor(2);
    expectBox(rounded, 10, 20, 60, 80);
    EXPECT_DOUBLE_EQ(rounded.getCornerRadius(), 10);
}

TEST(ShapeTest, ScaleKeepsCircleAndHexagonCentredAndUpdatesTheirBox) {
    Circle circle(10, 20, 30);
    circle.scale_factor(2);
    EXPECT_DOUBLE_EQ(circle.getRadius(), 60);
    EXPECT_TRUE(circle.contains(70, 20));
    expectBox(circle, -50, -40, 120, 120);

    Hexagon hexagon(10, 20, 30);
    hexagon.scale_factor(2);
    EXPECT_DOUBLE_EQ(hexagon.getRadius(), 60);
    EXPECT_TRUE(hexagon.contains(70, 20));
    expectBox(hexagon, -50, -40, 120, 120);
}

TEST(ShapeTest, ScaleKeepsALinesStartAndUpdatesItsBox) {
    Line line(10, 20, 30, 40);
    line.scale_factor(2);
    EXPECT_DOUBLE_EQ(line.getX1(), 10);
    EXPECT_DOUBLE_EQ(line.getX2(), 50);
    EXPECT_DOUBLE_EQ(line.getY2(), 60);
    expectBox(line, 10, 20, 40, 40);
}

TEST(ShapeTest, ScaleKeepsASketchsFirstPointAndUpdatesItsBox) {
    const auto shape = sketch();
    shape->scale_factor(2);
    EXPECT_DOUBLE_EQ(shape->getPoints()[0].x, 10);
    EXPECT_DOUBLE_EQ(shape->getPoints()[1].x, 50);
    expectBox(*shape, 10, 20, 40, 80);
}

TEST(ShapeTest, ScaleChangesATextsFontSizeAndStopsAtOne) {
    Text text(10, 20, "Hi");
    text.scale_factor(2);
    EXPECT_DOUBLE_EQ(text.getFontSize(), 32);
    EXPECT_DOUBLE_EQ(text.getWidth(), 0.6 * 32 * 2);

    text.scale_factor(0.001);
    EXPECT_DOUBLE_EQ(text.getFontSize(), 1);
}

TEST(ShapeTest, CloneMatchesTheOriginalInEverythingButId) {
    for (const Sample& sample : samples()) {
        GraphicsObject& shape = *sample.shape;
        shape.setStrokeColor("red");
        shape.setFillColor("blue");
        shape.setStrokeWidth(3);

        const auto copy = shape.clone();

        EXPECT_EQ(copy->getType(), shape.getType());
        EXPECT_EQ(withoutId(*copy), withoutId(shape));
        EXPECT_DOUBLE_EQ(copy->getWidth(), shape.getWidth()) << shape.getType();
        EXPECT_DOUBLE_EQ(copy->getHeight(), shape.getHeight()) << shape.getType();
    }
}

TEST(ShapeTest, EditingACloneLeavesTheOriginalAlone) {
    for (const Sample& sample : samples()) {
        const GraphicsObject& shape = *sample.shape;
        const std::string before = shape.toSVG();

        const auto copy = shape.clone();
        copy->move(5, 5);
        copy->scale_factor(2);
        copy->setStrokeColor("green");

        EXPECT_EQ(shape.toSVG(), before);
    }
}

TEST(ShapeTest, ToSvgWritesEachShape) {
    const auto expectSvg = [](GraphicsObject& shape, const std::string& expected) {
        shape.setId("a");
        EXPECT_EQ(shape.toSVG(), expected);
    };

    Rectangle rect(1, 2, 3, 4);
    expectSvg(rect, R"(<rect id="a" x="1" y="2" width="3" height="4" stroke="black" stroke-width="1" fill="white" />)");

    RoundedRectangle rounded(1, 2, 30, 40, 5);
    expectSvg(rounded,
              R"(<rect id="a" x="1" y="2" width="30" height="40" rx="5" ry="5" stroke="black" stroke-width="1" fill="white" />)");

    Circle circle(1, 2, 3);
    expectSvg(circle, R"(<circle id="a" cx="1" cy="2" r="3" stroke="black" stroke-width="1" fill="white" />)");

    Line line(1, 2, 3, 4);
    expectSvg(line, R"(<line id="a" x1="1" y1="2" x2="3" y2="4" stroke="black" stroke-width="1" />)");

    const auto freehand = sketch();
    expectSvg(*freehand, R"(<path id="a" d="M 10 20 L 30 60" stroke="black" stroke-width="1" fill="none" />)");

    Text text(1, 2, "a<b");
    expectSvg(text,
              R"(<text id="a" x="1" y="2" font-size="16" font-family="Arial" fill="white" stroke="black" stroke-width="1">a&lt;b</text>)");
}

TEST(ShapeTest, ToSvgWritesAHexagonsSixPoints) {
    Hexagon hexagon(0, 0, 2);
    hexagon.setId("a");

    const std::regex expected(
        R"(<polygon id="a" points="2,0( \S+,\S+){5}" stroke="black" stroke-width="1" fill="white" />)");
    EXPECT_TRUE(std::regex_match(hexagon.toSVG(), expected)) << hexagon.toSVG();
}
