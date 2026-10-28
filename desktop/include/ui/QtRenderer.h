#ifndef QTRENDERER_H
#define QTRENDERER_H

#include "core/Renderer.h"

class QPainter;

// Draws shapes through a QPainter.
class QtRenderer : public Renderer {
public:
    explicit QtRenderer(QPainter& painter) : painter_(painter) {}

    void drawRect(const RectGeom& rect, const Style& style) override;
    void drawEllipse(const EllipseGeom& ellipse, const Style& style) override;
    void drawPolyline(const std::vector<Point>& points, bool closed, const Style& style) override;
    void drawText(const TextGeom& text, const Style& style) override;

private:
    QPainter& painter_;
};

#endif // QTRENDERER_H
