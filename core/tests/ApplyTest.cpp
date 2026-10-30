#include <gtest/gtest.h>

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

// Adds a default-styled shape with `geometry` as "a", applies `op`, and checks it saves the same as `expected`.
template <typename Geometry>
void expectAfter(const Geometry& geometry, const Operation& op, std::unique_ptr<GraphicsObject> expected) {
    expected->setId("a");

    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {geometry, {}}}));
    ASSERT_TRUE(apply(diagram, op));
    EXPECT_EQ(diagram.find("a")->toSVG(), expected->toSVG());
}

std::unique_ptr<GraphicsObject> sketch(const std::vector<Point>& points) {
    auto shape = std::make_unique<FreehandSketch>();
    for (const Point& p : points) shape->addPoint(p.x, p.y);
    return shape;
}

std::unique_ptr<GraphicsObject> text(double x, double y, double fontSize) {
    auto shape = std::make_unique<Text>(x, y, "Hi");
    shape->setFrontSize(fontSize);
    return shape;
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

TEST(ApplyTest, MoveShiftsEveryKindOfShape) {
    const Move move{"a", 10, 20};
    expectAfter(RectGeom{1, 2, 3, 4}, move, std::make_unique<Rectangle>(11, 22, 3, 4));
    expectAfter(RectGeom{1, 2, 3, 4, 5}, move, std::make_unique<RoundedRectangle>(11, 22, 3, 4, 5));
    expectAfter(CircleGeom{{1, 2}, 3}, move, std::make_unique<Circle>(11, 22, 3));
    expectAfter(LineGeom{{1, 2}, {3, 4}}, move, std::make_unique<Line>(11, 22, 13, 24));
    expectAfter(HexagonGeom{{1, 2}, 3}, move, std::make_unique<Hexagon>(11, 22, 3));
    expectAfter(FreehandGeom{{{1, 2}, {3, 4}}}, move, sketch({{11, 22}, {13, 24}}));
    expectAfter(TextData{{1, 2}, "Hi", 16}, move, text(11, 22, 16));
}

TEST(ApplyTest, ResizeScalesEveryKindOfShape) {
    const Resize resize{"a", 2};
    expectAfter(RectGeom{1, 2, 3, 4}, resize, std::make_unique<Rectangle>(1, 2, 6, 8));
    expectAfter(RectGeom{1, 2, 3, 4, 5}, resize, std::make_unique<RoundedRectangle>(1, 2, 6, 8, 10));
    expectAfter(CircleGeom{{1, 2}, 3}, resize, std::make_unique<Circle>(1, 2, 6));
    expectAfter(LineGeom{{1, 2}, {3, 4}}, resize, std::make_unique<Line>(1, 2, 5, 6));
    expectAfter(HexagonGeom{{1, 2}, 3}, resize, std::make_unique<Hexagon>(1, 2, 6));
    expectAfter(FreehandGeom{{{1, 2}, {3, 4}}}, resize, sketch({{1, 2}, {5, 6}}));
    expectAfter(TextData{{1, 2}, "Hi", 16}, resize, text(1, 2, 32));
}

TEST(ApplyTest, MoveOrResizeOfAnUnknownIdFails) {
    Diagram diagram;

    for (const Operation& op : {Operation{Move{"nope", 1, 1}}, Operation{Resize{"nope", 2}}}) {
        const auto result = apply(diagram, op);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ApplyError::ShapeNotFound);
    }
}

TEST(ApplyTest, BadNumbersFailAndChangeNothing) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {RectGeom{1, 2, 3, 4}, {}}}));
    const std::string before = diagram.toSVG();

    for (const Operation& op : {Operation{Move{"a", nan, 0}}, Operation{Move{"a", 0, infinity}},
                                Operation{Resize{"a", 0}}, Operation{Resize{"a", -2}},
                                Operation{Resize{"a", nan}}, Operation{Resize{"a", infinity}}}) {
        const auto result = apply(diagram, op);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ApplyError::InvalidValue);
    }
    EXPECT_EQ(diagram.toSVG(), before);
}

TEST(ApplyTest, SetStyleChangesOnlyTheFieldsThatAreSet) {
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {RectGeom{}, redOnBlue}}));
    const GraphicsObject& shape = *diagram.find("a");

    ASSERT_TRUE(apply(diagram, SetStyle{"a", "green", std::nullopt, std::nullopt}));
    EXPECT_EQ(shape.getStrokeColor(), "green");
    EXPECT_EQ(shape.getFillColor(), "blue");
    EXPECT_DOUBLE_EQ(shape.getStrokeWidth(), 3);

    ASSERT_TRUE(apply(diagram, SetStyle{"a", std::nullopt, "yellow", std::nullopt}));
    EXPECT_EQ(shape.getStrokeColor(), "green");
    EXPECT_EQ(shape.getFillColor(), "yellow");
    EXPECT_DOUBLE_EQ(shape.getStrokeWidth(), 3);

    ASSERT_TRUE(apply(diagram, SetStyle{"a", std::nullopt, std::nullopt, 0.5}));
    EXPECT_EQ(shape.getStrokeColor(), "green");
    EXPECT_EQ(shape.getFillColor(), "yellow");
    EXPECT_DOUBLE_EQ(shape.getStrokeWidth(), 0.5);
}

TEST(ApplyTest, SetStyleOfAnUnknownIdFails) {
    Diagram diagram;

    const auto result = apply(diagram, SetStyle{"nope", "red", std::nullopt, std::nullopt});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::ShapeNotFound);
}

TEST(ApplyTest, SetStyleWithABadFieldChangesNothing) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {RectGeom{}, redOnBlue}}));
    const std::string before = diagram.toSVG();

    for (const SetStyle& op : {SetStyle{"a", "green", "yellow", -1}, SetStyle{"a", "green", "yellow", nan},
                               SetStyle{"a", "green", "x\" onload=\"y", 2},
                               SetStyle{"a", "x\"", "yellow", 2}}) {
        const auto result = apply(diagram, op);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ApplyError::InvalidValue);
    }
    EXPECT_EQ(diagram.toSVG(), before);
}

TEST(ApplyTest, AddWithAQuoteInAnIdOrColourFails) {
    Diagram diagram;

    for (const AddShape& op : {AddShape{"a\"", {RectGeom{}, {}}},
                               AddShape{"a", {RectGeom{}, Style{"x\"", "white", 1}}},
                               AddShape{"a", {RectGeom{}, Style{"black", "x\"", 1}}}}) {
        const auto result = apply(diagram, op);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ApplyError::InvalidShape);
    }
    EXPECT_EQ(diagram.getObjectCount(), 0);
}

namespace {

// A diagram with one shape of every kind, ids "rect", "rounded", ... in that drawing order.
Diagram oneOfEachShape() {
    Diagram diagram;
    const Style style{"red", "blue", 3};
    const std::vector<AddShape> shapes = {
        {"rect", {RectGeom{1, 2, 30, 40}, style}},
        {"rounded", {RectGeom{1, 2, 30, 40, 5}, style}},
        {"circle", {CircleGeom{{1, 2}, 3}, style}},
        {"line", {LineGeom{{1, 2}, {3, 4}}, style}},
        {"hexagon", {HexagonGeom{{1, 2}, 3}, style}},
        {"sketch", {FreehandGeom{{{1, 2}, {3, 4}}}, style}},
        {"text", {TextData{{1, 2}, "Hi", 20}, style}},
    };
    for (const AddShape& op : shapes) EXPECT_TRUE(apply(diagram, op));
    return diagram;
}

// Applies `op` then its inverse, and checks the diagram saves the same as before.
void expectUndoRestores(Diagram& diagram, const Operation& op) {
    const std::string before = diagram.toSVG();

    const auto inverse = invert(diagram, op);
    ASSERT_TRUE(inverse);
    ASSERT_TRUE(apply(diagram, op));
    ASSERT_TRUE(apply(diagram, *inverse));

    EXPECT_EQ(diagram.toSVG(), before);
}

const char* const allIds[] = {"rect", "rounded", "circle", "line", "hexagon", "sketch", "text"};

}  // namespace

TEST(InvertTest, UndoingAnAddRemovesTheShape) {
    Diagram diagram = oneOfEachShape();
    expectUndoRestores(diagram, AddShape{"new", {CircleGeom{{1, 2}, 3}, {}}});
    EXPECT_EQ(diagram.find("new"), nullptr);
}

TEST(InvertTest, UndoingADeleteRestoresEveryKindInItsPlace) {
    for (const char* id : allIds) {
        Diagram diagram = oneOfEachShape();
        expectUndoRestores(diagram, DeleteShape{id});
    }
}

TEST(InvertTest, UndoingAMoveRestoresEveryKind) {
    for (const char* id : allIds) {
        Diagram diagram = oneOfEachShape();
        expectUndoRestores(diagram, Move{id, 10, -20});
    }
}

TEST(InvertTest, UndoingAResizeRestoresEveryKind) {
    for (const char* id : allIds) {
        Diagram diagram = oneOfEachShape();
        expectUndoRestores(diagram, Resize{id, 2});
    }
}

TEST(InvertTest, UndoingAResizeIsCloseWhenTheFactorRounds) {
    Diagram diagram = oneOfEachShape();
    const Operation op = Resize{"rect", 3.3};

    const auto inverse = invert(diagram, op);
    ASSERT_TRUE(inverse);
    ASSERT_TRUE(apply(diagram, op));
    ASSERT_TRUE(apply(diagram, *inverse));

    EXPECT_NEAR(diagram.find("rect")->getWidth(), 30, 1e-9);
}

TEST(InvertTest, UndoingAShrinkStoppedByTheTextLimitRestoresTheSize) {
    Diagram diagram = oneOfEachShape();
    expectUndoRestores(diagram, Resize{"text", 0.01});
}

TEST(InvertTest, UndoingSetStyleRestoresEveryField) {
    Diagram diagram = oneOfEachShape();
    expectUndoRestores(diagram, SetStyle{"rect", "green", "yellow", 7});
}

TEST(InvertTest, SetStyleInverseOnlyHasTheFieldsThatWereSet) {
    const Diagram diagram = oneOfEachShape();

    const auto inverse = invert(diagram, SetStyle{"rect", "green", std::nullopt, std::nullopt});

    ASSERT_TRUE(inverse);
    const auto& style = std::get<SetStyle>(*inverse);
    EXPECT_EQ(style.stroke, "red");
    EXPECT_EQ(style.fill, std::nullopt);
    EXPECT_EQ(style.strokeWidth, std::nullopt);
}

TEST(InvertTest, InvertingADeleteTwiceDeletesAgain) {
    Diagram diagram = oneOfEachShape();
    const Operation op = DeleteShape{"line"};

    const auto undo = invert(diagram, op);
    ASSERT_TRUE(undo);
    ASSERT_TRUE(apply(diagram, op));
    const auto redo = invert(diagram, *undo);
    ASSERT_TRUE(redo);
    ASSERT_TRUE(apply(diagram, *undo));
    ASSERT_NE(diagram.find("line"), nullptr);

    ASSERT_TRUE(apply(diagram, *redo));
    EXPECT_EQ(diagram.find("line"), nullptr);
}

TEST(InvertTest, UnknownIdsFail) {
    const Diagram diagram;

    for (const Operation& op : {Operation{DeleteShape{"nope"}}, Operation{Move{"nope", 1, 1}},
                                Operation{Resize{"nope", 2}},
                                Operation{SetStyle{"nope", "red", std::nullopt, std::nullopt}}}) {
        const auto inverse = invert(diagram, op);
        ASSERT_FALSE(inverse);
        EXPECT_EQ(inverse.error(), ApplyError::ShapeNotFound);
    }
}

TEST(InvertTest, AddWithATakenIdFails) {
    const Diagram diagram = oneOfEachShape();

    const auto inverse = invert(diagram, AddShape{"rect", {RectGeom{}, {}}});

    ASSERT_FALSE(inverse);
    EXPECT_EQ(inverse.error(), ApplyError::DuplicateId);
}

TEST(InvertTest, ResizeThatCouldNeverBeUndoneFails) {
    const Diagram diagram = oneOfEachShape();

    for (const double factor : {0.0, -1.0, 1e-320}) {
        const auto inverse = invert(diagram, Resize{"rect", factor});
        ASSERT_FALSE(inverse);
        EXPECT_EQ(inverse.error(), ApplyError::InvalidValue);
    }
}

TEST(ApplyTest, AddAtAPositionOutOfRangeFails) {
    Diagram diagram;

    const auto result = apply(diagram, AddShape{"a", {RectGeom{}, {}}, 1});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::InvalidShape);
    EXPECT_EQ(diagram.getObjectCount(), 0);
}

TEST(ApplyTest, DeleteOfAnUnknownIdFailsAndChangesNothing) {
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {RectGeom{}, {}}}));

    const auto result = apply(diagram, DeleteShape{"nope"});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::ShapeNotFound);
    EXPECT_EQ(diagram.getObjectCount(), 1);
}
