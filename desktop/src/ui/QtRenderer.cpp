#include "ui/QtRenderer.h"

#include <QBrush>
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>

namespace {

QColor toQColor(const std::string& color) {
    return QColor(QString::fromStdString(color));
}

// Sharp (mitred) corners, so thick outlines keep the shape's corners square.
QPen toPen(const Style& style) {
    QPen pen(toQColor(style.stroke));
    pen.setWidthF(style.strokeWidth);
    pen.setJoinStyle(Qt::MiterJoin);
    return pen;
}

QBrush toBrush(const Style& style) {
    if (style.fill == "none") return Qt::NoBrush;
    return QBrush(toQColor(style.fill));
}

}  // namespace

void QtRenderer::drawRect(const RectGeom& rect, const Style& style) {
    painter_.setPen(toPen(style));
    painter_.setBrush(toBrush(style));
    const QRectF area(rect.x, rect.y, rect.width, rect.height);
    if (rect.cornerRadius > 0) {
        painter_.drawRoundedRect(area, rect.cornerRadius, rect.cornerRadius);
    } else {
        painter_.drawRect(area);
    }
}

void QtRenderer::drawEllipse(const EllipseGeom& ellipse, const Style& style) {
    painter_.setPen(toPen(style));
    painter_.setBrush(toBrush(style));
    painter_.drawEllipse(QPointF(ellipse.center.x, ellipse.center.y), ellipse.rx, ellipse.ry);
}

void QtRenderer::drawPolyline(const std::vector<Point>& points, bool closed, const Style& style) {
    QPolygonF polygon;
    polygon.reserve(static_cast<int>(points.size()));
    for (const Point& p : points) polygon << QPointF(p.x, p.y);

    painter_.setPen(toPen(style));
    if (closed) {
        painter_.setBrush(toBrush(style));
        painter_.drawPolygon(polygon);
    } else {
        painter_.drawPolyline(polygon);
    }
}

void QtRenderer::drawText(const TextGeom& text, const Style& style) {
    QFont font(QString::fromStdString(text.fontFamily.empty() ? "Arial" : text.fontFamily));
    font.setPointSizeF(text.fontSize);

    // addText() places the baseline at y, so move down by the ascent to start at the top.
    QPainterPath path;
    path.addText(text.topLeft.x, text.topLeft.y + QFontMetricsF(font).ascent(), font,
                 QString::fromStdString(text.text));

    QPen pen = toPen(style);
    pen.setJoinStyle(Qt::BevelJoin);  // mitred joins spike on the sharp corners of glyphs
    painter_.fillPath(path, toBrush(style));
    painter_.strokePath(path, pen);
}
