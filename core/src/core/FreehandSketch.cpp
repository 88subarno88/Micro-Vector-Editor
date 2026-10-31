#include "core/FreehandSketch.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "core/Renderer.h"

FreehandSketch::FreehandSketch() : GraphicsObject(0, 0, 0, 0) {}  // (x,y,width,height)

// an open polyline, so it is never filled
void FreehandSketch::draw(Renderer& renderer) const {
    if (!points_.empty()) renderer.drawPolyline(points_, false, style());
}

std::string FreehandSketch::toSVG() const {
    if (points_.empty()) {
        return "";
    }
    std::ostringstream svg;

    svg << "<path " << idAttribute() << "d=\"";
    svg << "M " << points_[0].x << " " << points_[0].y;
    for (size_t i = 1; i < points_.size(); i++) {
        svg << " L " << points_[i].x << " " << points_[i].y;  // get all pts
    }
    svg << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"none\"";  // freehand has none to fill in
    svg << " />";

    return svg.str();
}

// create the deep copy of the object | used for copy pasting purpose
std::unique_ptr<GraphicsObject> FreehandSketch::clone() const {
    auto copy = std::make_unique<FreehandSketch>();

    // copy all points
    for (const Point& p : points_) {
        copy->addPoint(p.x, p.y);
    }

    // copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setStrokeWidth(getStrokeWidth());

    return copy;
}

// get type,here "FreehandSketch"
std::string FreehandSketch::getType() const { return "FreehandSketch"; }

// add a point to the freehand sketch
void FreehandSketch::addPoint(double x, double y) {
    points_.push_back({x, y});
    updateBoundingBox();
}

// get all points_ in the freehand sketch
const std::vector<Point>& FreehandSketch::getPoints() const { return points_; }
// answers(T/F) if we have (x,y) in our freehand sketch
bool FreehandSketch::contains(double x, double y) const {
    return (x >= getX() && x <= getX() + getWidth() && y >= getY() && y <= getY() + getHeight());
}

void FreehandSketch::updateBoundingBox() {
    if (points_.empty()) {
        return;
    }

    double minX = points_[0].x;
    double maxX = points_[0].x;
    double minY = points_[0].y;
    double maxY = points_[0].y;

    for (const Point& p : points_) {
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }

    setX(minX);
    setY(minY);
    setWidth(maxX - minX);
    setHeight(maxY - minY);
}

// move
void FreehandSketch::move(double dx, double dy) {
    for (auto& point : points_) {
        point.x += dx;
        point.y += dy;
    }
    // update the box
    GraphicsObject::move(dx, dy);
}
// resize
void FreehandSketch::scaleBy(double factor) {
    if (points_.empty()) return;

    // use first point as pivot
    double pvtX = points_[0].x;
    double pvtY = points_[0].y;

    // iter through points
    for (auto& p : points_) {
        double dx = p.x - pvtX;
        double dy = p.y - pvtY;
        p.x = pvtX + (dx * factor);
        p.y = pvtY + (dy * factor);
    }
    updateBoundingBox();
}