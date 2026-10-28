#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "core/Geometry.h"

// What shapes draw onto. The desktop app implements it with QPainter; a web build can
// implement it with an HTML canvas, and tests can record the calls.
class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void drawRect(const RectGeom& rect, const Style& style) = 0;
    virtual void drawEllipse(const EllipseGeom& ellipse, const Style& style) = 0;
    // Joins the points with straight lines; `closed` joins the last point back to the
    // first and fills the inside.
    virtual void drawPolyline(const std::vector<Point>& points, bool closed, const Style& style) = 0;
    virtual void drawText(const TextGeom& text, const Style& style) = 0;
};

#endif // RENDERER_H
