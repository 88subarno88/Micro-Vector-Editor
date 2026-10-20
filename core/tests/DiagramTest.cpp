#include <gtest/gtest.h>

#include <memory>

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
