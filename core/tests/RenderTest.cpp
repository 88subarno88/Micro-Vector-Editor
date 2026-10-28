#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Renderer.h"
#include "core/RoundedRectangle.h"

namespace {

// Remembers what a shape asked to draw.
class RecordingRenderer : public Renderer {
public:
    std::vector<std::string> calls;
    RectGeom rect;
    EllipseGeom ellipse;
    Style style;

    void drawRect(const RectGeom& r, const Style& s) override {
        calls.push_back("rect");
        rect = r;
        style = s;
    }
    void drawEllipse(const EllipseGeom& e, const Style& s) override {
        calls.push_back("ellipse");
        ellipse = e;
        style = s;
    }
    void drawPolyline(const std::vector<Point>&, bool, const Style& s) override {
        calls.push_back("polyline");
        style = s;
    }
    void drawText(const TextGeom&, const Style& s) override {
        calls.push_back("text");
        style = s;
    }
};

}  // namespace

TEST(RenderTest, RectangleDrawsOneSharpRect) {
    RecordingRenderer renderer;
    Rectangle(1, 2, 30, 40).draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"rect"});
    EXPECT_DOUBLE_EQ(renderer.rect.x, 1);
    EXPECT_DOUBLE_EQ(renderer.rect.y, 2);
    EXPECT_DOUBLE_EQ(renderer.rect.width, 30);
    EXPECT_DOUBLE_EQ(renderer.rect.height, 40);
    EXPECT_DOUBLE_EQ(renderer.rect.cornerRadius, 0);
}

TEST(RenderTest, RoundedRectangleDrawsRectWithCornerRadius) {
    RecordingRenderer renderer;
    RoundedRectangle(0, 0, 50, 50, 8).draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"rect"});
    EXPECT_DOUBLE_EQ(renderer.rect.cornerRadius, 8);
}

TEST(RenderTest, CircleDrawsEllipseAroundItsCentre) {
    RecordingRenderer renderer;
    Circle(10, 20, 5).draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"ellipse"});
    EXPECT_DOUBLE_EQ(renderer.ellipse.center.x, 10);
    EXPECT_DOUBLE_EQ(renderer.ellipse.center.y, 20);
    EXPECT_DOUBLE_EQ(renderer.ellipse.rx, 5);
    EXPECT_DOUBLE_EQ(renderer.ellipse.ry, 5);
}

TEST(RenderTest, ShapePassesItsStyle) {
    Rectangle shape(0, 0, 1, 1);
    shape.setStrokeColor("red");
    shape.setFillColor("none");
    shape.setStrokeWidth(3);

    RecordingRenderer renderer;
    shape.draw(renderer);

    EXPECT_EQ(renderer.style.stroke, "red");
    EXPECT_EQ(renderer.style.fill, "none");
    EXPECT_DOUBLE_EQ(renderer.style.strokeWidth, 3);
}
