#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "ops/CommandStack.h"
#include "parser/SVGParser.h"

using namespace vec;

namespace {

// A diagram holding one 10x20 rectangle "a" at (1, 2), added outside the history.
class CommandStackTest : public ::testing::Test {
protected:
    CommandStackTest() { EXPECT_TRUE(apply(diagram, addRect("a"))); }

    static AddShape addRect(const ShapeId& id) { return {id, {RectGeom{1, 2, 10, 20}, {}}}; }

    GraphicsObject& shape() { return *diagram.find("a"); }

    Diagram diagram;
    CommandStack stack;
};

}  // namespace

TEST_F(CommandStackTest, StartsWithNothingToUndoOrRedo) {
    EXPECT_FALSE(stack.canUndo());
    EXPECT_FALSE(stack.canRedo());
    EXPECT_TRUE(stack.undo(diagram));
    EXPECT_TRUE(stack.redo(diagram));
    EXPECT_EQ(diagram.getObjectCount(), 1);
}

TEST_F(CommandStackTest, UndoAndRedoEveryKindOfOp) {
    const std::string start = diagram.toSVG();
    const Operation ops[] = {addRect("b"), DeleteShape{"a"}, Move{"a", 5, 7}, Resize{"a", 2},
                             SetStyle{"a", "red", "blue", 3}};

    for (const Operation& op : ops) {
        ASSERT_TRUE(stack.execute(diagram, op));
        const std::string after = diagram.toSVG();
        EXPECT_NE(after, start);

        ASSERT_TRUE(stack.undo(diagram));
        EXPECT_EQ(diagram.toSVG(), start);

        ASSERT_TRUE(stack.redo(diagram));
        EXPECT_EQ(diagram.toSVG(), after);

        ASSERT_TRUE(stack.undo(diagram));
    }
}

TEST_F(CommandStackTest, UndoesAndRedoesInOrder) {
    ASSERT_TRUE(stack.execute(diagram, Move{"a", 1, 0}));
    ASSERT_TRUE(stack.execute(diagram, Move{"a", 10, 0}));

    ASSERT_TRUE(stack.undo(diagram));
    EXPECT_DOUBLE_EQ(shape().getX(), 2);
    ASSERT_TRUE(stack.undo(diagram));
    EXPECT_DOUBLE_EQ(shape().getX(), 1);
    EXPECT_FALSE(stack.canUndo());

    ASSERT_TRUE(stack.redo(diagram));
    ASSERT_TRUE(stack.redo(diagram));
    EXPECT_DOUBLE_EQ(shape().getX(), 12);
    EXPECT_FALSE(stack.canRedo());
}

TEST_F(CommandStackTest, ExecutingAfterAnUndoDropsTheRedoHistory) {
    ASSERT_TRUE(stack.execute(diagram, Move{"a", 1, 0}));
    ASSERT_TRUE(stack.undo(diagram));

    ASSERT_TRUE(stack.execute(diagram, Move{"a", 10, 0}));

    EXPECT_FALSE(stack.canRedo());
    EXPECT_DOUBLE_EQ(shape().getX(), 11);
}

TEST_F(CommandStackTest, AFailedOpIsNotRecordedAndKeepsTheRedoHistory) {
    ASSERT_TRUE(stack.execute(diagram, Move{"a", 1, 0}));
    ASSERT_TRUE(stack.undo(diagram));

    const auto result = stack.execute(diagram, Move{"nope", 1, 0});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ApplyError::ShapeNotFound);
    EXPECT_FALSE(stack.canUndo());
    EXPECT_TRUE(stack.canRedo());
}

TEST_F(CommandStackTest, UndoingADeleteKeepsTheStackingOrder) {
    ASSERT_TRUE(apply(diagram, addRect("b")));
    const std::string start = diagram.toSVG();

    ASSERT_TRUE(stack.execute(diagram, DeleteShape{"a"}));
    ASSERT_TRUE(stack.undo(diagram));

    EXPECT_EQ(diagram.toSVG(), start);
}

TEST_F(CommandStackTest, EarlierOpStillUndoesAfterADeleteAndRestore) {
    ASSERT_TRUE(stack.execute(diagram, Move{"a", 5, 0}));
    ASSERT_TRUE(stack.execute(diagram, DeleteShape{"a"}));

    ASSERT_TRUE(stack.undo(diagram));
    ASSERT_TRUE(stack.undo(diagram));

    EXPECT_DOUBLE_EQ(shape().getX(), 1);
}

TEST_F(CommandStackTest, ClearForgetsTheHistoryButNotTheDiagram) {
    ASSERT_TRUE(stack.execute(diagram, addRect("b")));

    stack.clear();

    EXPECT_FALSE(stack.canUndo());
    EXPECT_FALSE(stack.canRedo());
    EXPECT_NE(diagram.find("b"), nullptr);
}

TEST(CommandStackImportTest, DeleteOfAnImportedShapeCanBeUndone) {
    auto diagram = SVGParser::parse_SVG(R"(<rect id="a" stroke='x"y' fill='x"y'/>)");
    ASSERT_TRUE(diagram);
    CommandStack stack;

    ASSERT_TRUE(stack.execute(*diagram, DeleteShape{"a"}));
    ASSERT_TRUE(stack.undo(*diagram));

    EXPECT_NE(diagram->find("a"), nullptr);
}
