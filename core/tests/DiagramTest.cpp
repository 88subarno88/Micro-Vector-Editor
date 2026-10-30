#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "core/Diagram.h"
#include "core/Rectangle.h"

TEST(DiagramTest, StartsEmpty) {
    Diagram diagram;

    EXPECT_EQ(diagram.getObjectCount(), 0);
    EXPECT_TRUE(diagram.getShapes().empty());
}

TEST(DiagramTest, AddAndClear) {
    Diagram diagram;
    diagram.addObject(std::make_unique<Rectangle>(0, 0, 10, 20));
    diagram.addObject(std::make_unique<Rectangle>(5, 5, 1, 1));

    EXPECT_EQ(diagram.getObjectCount(), 2);

    diagram.clearallObjects();
    EXPECT_EQ(diagram.getObjectCount(), 0);
}

namespace {

// Adds a rectangle and returns its id.
ShapeId addRect(Diagram& diagram) {
    auto shape = std::make_unique<Rectangle>();
    const ShapeId id = shape->getId();
    diagram.addObject(std::move(shape));
    return id;
}

std::vector<ShapeId> idsInOrder(const Diagram& diagram) {
    std::vector<ShapeId> ids;
    for (const auto& shape : diagram.getShapes()) ids.push_back(shape->getId());
    return ids;
}

}  // namespace

TEST(DiagramTest, FindsShapeById) {
    Diagram diagram;
    addRect(diagram);
    const ShapeId id = addRect(diagram);

    const GraphicsObject* found = diagram.find(id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), id);
}

TEST(DiagramTest, FindReturnsNullForUnknownId) {
    Diagram diagram;
    addRect(diagram);

    EXPECT_EQ(diagram.find("s_nope"), nullptr);
}

TEST(DiagramTest, RemoveByIdReturnsTheShape) {
    Diagram diagram;
    const ShapeId id = addRect(diagram);

    const auto removed = diagram.removeObject(id);

    ASSERT_NE(removed, nullptr);
    EXPECT_EQ(removed->getId(), id);
    EXPECT_EQ(diagram.getObjectCount(), 0);
    EXPECT_EQ(diagram.find(id), nullptr);
}

TEST(DiagramTest, RemoveUnknownIdChangesNothing) {
    Diagram diagram;
    addRect(diagram);

    EXPECT_EQ(diagram.removeObject("s_nope"), nullptr);
    EXPECT_EQ(diagram.getObjectCount(), 1);
}

TEST(DiagramTest, RemovingFromTheMiddleKeepsOrderAndLookups) {
    Diagram diagram;
    const ShapeId a = addRect(diagram);
    const ShapeId b = addRect(diagram);
    const ShapeId c = addRect(diagram);
    const ShapeId d = addRect(diagram);

    diagram.removeObject(b);

    EXPECT_EQ(idsInOrder(diagram), (std::vector<ShapeId>{a, c, d}));
    for (const ShapeId& id : {a, c, d}) {
        ASSERT_NE(diagram.find(id), nullptr);
        EXPECT_EQ(diagram.find(id)->getId(), id);
    }
}

TEST(DiagramTest, ClearForgetsIds) {
    Diagram diagram;
    const ShapeId id = addRect(diagram);

    diagram.clearallObjects();

    EXPECT_EQ(diagram.find(id), nullptr);
}

TEST(DiagramTest, MovedDiagramKeepsLookups) {
    Diagram original;
    const ShapeId id = addRect(original);

    Diagram moved = std::move(original);

    ASSERT_NE(moved.find(id), nullptr);
    EXPECT_EQ(moved.find(id)->getId(), id);
}
