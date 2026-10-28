#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/Circle.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/Renderer.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"

namespace {

// Remembers what a shape asked to draw.
class RecordingRenderer : public Renderer {
public:
    std::vector<std::string> calls;
    RectGeom rect;
    EllipseGeom ellipse;
    std::vector<Point> points;
    bool closed = false;
    TextGeom text;
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
    void drawPolyline(const std::vector<Point>& p, bool c, const Style& s) override {
        calls.push_back("polyline");
        points = p;
        closed = c;
        style = s;
    }
    void drawText(const TextGeom& t, const Style& s) override {
        calls.push_back("text");
        text = t;
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

TEST(RenderTest, LineDrawsOpenTwoPointPolyline) {
    RecordingRenderer renderer;
    Line(1, 2, 3, 4).draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"polyline"});
    EXPECT_FALSE(renderer.closed);
    ASSERT_EQ(renderer.points.size(), 2u);
    EXPECT_DOUBLE_EQ(renderer.points[0].x, 1);
    EXPECT_DOUBLE_EQ(renderer.points[1].y, 4);
}

TEST(RenderTest, HexagonDrawsClosedPolylineOfSixCorners) {
    RecordingRenderer renderer;
    Hexagon(100, 50, 10).draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"polyline"});
    EXPECT_TRUE(renderer.closed);
    ASSERT_EQ(renderer.points.size(), 6u);
    EXPECT_DOUBLE_EQ(renderer.points[0].x, 110);  // rightmost corner
    EXPECT_DOUBLE_EQ(renderer.points[0].y, 50);
    EXPECT_DOUBLE_EQ(renderer.points[3].x, 90);   // leftmost corner
}

TEST(RenderTest, FreehandDrawsOpenPolylineThroughItsPoints) {
    FreehandSketch sketch;
    sketch.addPoint(1, 1);
    sketch.addPoint(2, 3);
    sketch.addPoint(4, 2);

    RecordingRenderer renderer;
    sketch.draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"polyline"});
    EXPECT_FALSE(renderer.closed);
    EXPECT_EQ(renderer.points.size(), 3u);
}

TEST(RenderTest, EmptyFreehandDrawsNothing) {
    RecordingRenderer renderer;
    FreehandSketch().draw(renderer);

    EXPECT_TRUE(renderer.calls.empty());
}

TEST(RenderTest, TextDrawsItsContentAndFont) {
    Text text(5, 6, "Hi");
    text.setFrontSize(24);

    RecordingRenderer renderer;
    text.draw(renderer);

    ASSERT_EQ(renderer.calls, std::vector<std::string>{"text"});
    EXPECT_DOUBLE_EQ(renderer.text.topLeft.x, 5);
    EXPECT_DOUBLE_EQ(renderer.text.topLeft.y, 6);
    EXPECT_EQ(renderer.text.text, "Hi");
    EXPECT_EQ(renderer.text.fontFamily, "Arial");
    EXPECT_DOUBLE_EQ(renderer.text.fontSize, 24);
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
