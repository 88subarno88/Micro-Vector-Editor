#include "core/FreehandSketch.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

FreehandSketch::FreehandSketch(): GraphicsObject(0,0,0,0 ){}      // x, y, width, height
      
// Destructor
FreehandSketch::~FreehandSketch() {}

// Drawing the circle using QPainter.
void FreehandSketch::draw(QPainter* painter) {
    (void)painter;  // Suppress unused parameter warning
}

std::string FreehandSketch::toSVG() const {  
    if (points_.empty()) { return "";}
    std::ostringstream svg;
    // Start path tag
    svg << "<path d=\"";
    // First point - Move to (M)
    svg << "M " << points_[0].x << " " << points_[0].y;
    // All other points - Line to (L)
    for (size_t i = 1; i < points_.size(); i++) {
        svg << " L " << points_[i].x << " " << points_[i].y;
    }
    // Close path data
    svg << "\" ";
    // Add styling
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"none\"";  // Freehand paths usually have no fill
    svg << " />";
    return svg.str();
}
//Create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> FreehandSketch::clone() const {
    auto copy = std::make_unique<FreehandSketch>();

    // Copy all points
    for (const Point& p : points_) {
        copy->addPoint(p.x, p.y);
    }
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
    return copy;
}
//Get type,here "FreehandSketch"
std::string FreehandSketch::getType() const {
    return "FreehandSketch";
}
// Add a point to the freehand sketch
void FreehandSketch::addPoint(double x, double y) {
    points_.push_back(Point(x,y));
    updateBoundingBox();
}
// Clear all points from the freehand sketch
void FreehandSketch::clearPoints() {
    points_.clear();
    // Reset bounding box
    setX(0);
    setY(0);
    setWidth(0);
    setHeight(0);
}
// Get the number of points in the freehand sketch
int FreehandSketch::getPointCount() const {
    return points_.size();
}

// Get all points in the freehand sketch
const std::vector<Point>& FreehandSketch::getPoints() const {
    return points_;
}
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