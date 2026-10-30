#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <utility>

#include "commands/AddCommand.h"
#include "commands/ChangeStrokewidthcommand.h"
#include "commands/ChangecolorCommand.h"
#include "commands/CommandStack.h"
#include "commands/CreateCommand.h"
#include "commands/DeleteCommand.h"
#include "commands/ModifyCommand.h"
#include "commands/MoveCommand.h"
#include "commands/ResizeCommand.h"
#include "core/Diagram.h"
#include "core/Rectangle.h"

namespace {

// A diagram holding one 10x20 rectangle at (1, 2).
class CommandTest : public ::testing::Test {
protected:
    CommandTest() {
        auto shape = std::make_unique<Rectangle>(1, 2, 10, 20);
        id = shape->getId();
        diagram.addObject(std::move(shape));
    }

    GraphicsObject& shape() { return *diagram.find(id); }

    Diagram diagram;
    ShapeId id;
    CommandStack stack;
};

}  // namespace

TEST_F(CommandTest, AddUndoRedoKeepsTheSameShape) {
    auto added = std::make_unique<Rectangle>();
    const ShapeId addedId = added->getId();

    stack.execute(std::make_unique<AddCommand>(&diagram, std::move(added)));
    EXPECT_NE(diagram.find(addedId), nullptr);

    stack.undo();
    EXPECT_EQ(diagram.find(addedId), nullptr);

    stack.redo();
    EXPECT_NE(diagram.find(addedId), nullptr);
}

TEST_F(CommandTest, CreateUndoRedoKeepsTheSameShape) {
    auto created = std::make_unique<Rectangle>();
    const ShapeId createdId = created->getId();

    stack.execute(std::make_unique<CreateCommand>(&diagram, std::move(created)));
    EXPECT_NE(diagram.find(createdId), nullptr);

    stack.undo();
    EXPECT_EQ(diagram.find(createdId), nullptr);

    stack.redo();
    EXPECT_NE(diagram.find(createdId), nullptr);
}

TEST_F(CommandTest, DeleteUndoRestoresTheShape) {
    stack.execute(std::make_unique<DeleteCommand>(&diagram, id));
    EXPECT_EQ(diagram.find(id), nullptr);

    stack.undo();
    ASSERT_NE(diagram.find(id), nullptr);
    EXPECT_DOUBLE_EQ(shape().getX(), 1);
}

TEST_F(CommandTest, MoveUndoRedo) {
    stack.execute(std::make_unique<MoveCommand>(&diagram, id, 5, 7));
    EXPECT_DOUBLE_EQ(shape().getX(), 6);
    EXPECT_DOUBLE_EQ(shape().getY(), 9);

    stack.undo();
    EXPECT_DOUBLE_EQ(shape().getX(), 1);

    stack.redo();
    EXPECT_DOUBLE_EQ(shape().getX(), 6);
}

TEST_F(CommandTest, ResizeUndoRestoresTheSize) {
    stack.execute(std::make_unique<ResizeCommand>(&diagram, id, 2));
    EXPECT_DOUBLE_EQ(shape().getWidth(), 20);

    stack.undo();
    EXPECT_DOUBLE_EQ(shape().getWidth(), 10);
}

TEST_F(CommandTest, ChangeColorUndoRestoresFillAndStroke) {
    stack.execute(std::make_unique<ChangeColorCommand>(&diagram, id, "red", true));
    stack.execute(std::make_unique<ChangeColorCommand>(&diagram, id, "blue", false));
    EXPECT_EQ(shape().getFillColor(), "red");
    EXPECT_EQ(shape().getStrokeColor(), "blue");

    stack.undo();
    EXPECT_EQ(shape().getStrokeColor(), "black");
    EXPECT_EQ(shape().getFillColor(), "red");

    stack.undo();
    EXPECT_EQ(shape().getFillColor(), "white");
}

TEST_F(CommandTest, ChangeStrokeWidthUndoRestoresTheWidth) {
    stack.execute(std::make_unique<ChangeStrokeWidthCommand>(&diagram, id, 4));
    EXPECT_DOUBLE_EQ(shape().getStrokeWidth(), 4);

    stack.undo();
    EXPECT_DOUBLE_EQ(shape().getStrokeWidth(), 1);
}

TEST_F(CommandTest, ModifyUndoRestoresEverything) {
    stack.execute(std::make_unique<ModifyCommand>(&diagram, id, "red", "blue", 3));
    EXPECT_EQ(shape().getStrokeColor(), "red");

    stack.undo();
    EXPECT_EQ(shape().getStrokeColor(), "black");
    EXPECT_EQ(shape().getFillColor(), "white");
    EXPECT_DOUBLE_EQ(shape().getStrokeWidth(), 1);
}

TEST_F(CommandTest, EarlierCommandStillWorksAfterDeleteAndRestore) {
    stack.execute(std::make_unique<MoveCommand>(&diagram, id, 5, 0));
    stack.execute(std::make_unique<DeleteCommand>(&diagram, id));

    stack.undo();  // restore
    stack.undo();  // move back

    EXPECT_DOUBLE_EQ(shape().getX(), 1);
}

TEST_F(CommandTest, CommandsOnAMissingShapeDoNothing) {
    Diagram empty;

    MoveCommand move(&empty, id, 1, 1);
    ResizeCommand resize(&empty, id, 2);
    ChangeColorCommand color(&empty, id, "red", true);
    ChangeStrokeWidthCommand width(&empty, id, 2);
    ModifyCommand modify(&empty, id, "red", "blue", 2);
    DeleteCommand erase(&empty, id);

    for (Command* command : std::initializer_list<Command*>{&move, &resize, &color, &width, &modify, &erase}) {
        command->execute();
        command->undo();
    }
    EXPECT_EQ(empty.getObjectCount(), 0);
}
