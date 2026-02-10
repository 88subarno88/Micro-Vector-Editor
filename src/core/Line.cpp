#include "core/Line.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>

//Qt includes
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPointF>

/**
 * Line class 
 * ------- represents a line segment
 * ------- defined by start point (x1, y1) and end point (x2, y2)
 */
Line::Line(double x1, double y1, double x2, double y2): 
    GraphicsObject(0,0,0,0), x1_(x1),y1_(y1),x2_(x2),y2_(y2) {
   updateBoundingBox();   
}
// Destructor
Line::~Line() {}
// Drawing the line using QPainter
void Line::draw(QPainter* painter) {
    if (!painter) return;

    QColor sColor(QString::fromStdString(getStrokeColor()));
    QPen pen(sColor);

    pen.setWidthF(getStrokeWidth());
    painter->setPen(pen);
    
    // Lines don't usually have a "fill", so we ignore brush
    painter->drawLine(QPointF(x1_, y1_), QPointF(x2_, y2_));
}

/**
 * Convert line to SVG format. It returns <line x1="..." y1="..." x2="..." y2="..." stroke="..." />
 */
std::string Line::toSVG() const {
    std::ostringstream svg;
    svg << "<line "; svg << "x1=\"" << x1_ << "\" ";svg << "y1=\"" << y1_ << "\" ";svg << "x2=\"" << x2_ << "\" ";svg << "y2=\"" << y2_ << "\" ";svg << "stroke=\"" << getStrokeColor() << "\" ";svg << "stroke-width=\"" << getStrokeWidth() << "\""; svg << " />";   
    return svg.str();
}
// Create the deep copy of the line | used for copy pasting purpose
std::unique_ptr<GraphicsObject> Line::clone() const {
    auto copy = std::make_unique<Line>(x1_, y1_,x2_, y2_);
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor()); copy->setStrokeWidth(getStrokeWidth()); copy->setSelected(isSelected());
    return copy;
}
// Get type, here "Line"
std::string Line::getType() const {
    return "Line";
}
// Set x coordinate of start point
void Line::setX1(double x) {
    x1_ =x; updateBoundingBox();
}
// Set y coordinate of start point
void Line::setY1(double y) {
    y1_ =y; updateBoundingBox();
}
// Set x coordinate of end point
void Line::setX2(double x) {
    x2_ =x; updateBoundingBox();
}
// Set y coordinate of end point
void Line::setY2(double y) {
    y2_ =y; updateBoundingBox();
}
// Set both coordinates of start point
void Line::setStartPoint(double x, double y) {
    x1_ =x;y1_ =y; updateBoundingBox();
}
// Set both coordinates of end point
void Line::setEndPoint(double x, double y) {
    x2_ =x; y2_ =y; updateBoundingBox();
}
/**
 * Check if a point is near the line basically within tolerance.
 * Find it by using distance from point to line segment formula.
 */
bool Line::contains(double x, double y) const {
    // Tolerance for hit detection in px , let assume our fault tolerance is 4px
    const double tolerance =25.0;
    // squared Length of line segment 
    double lengthSquared = (x2_- x1_)*(x2_- x1_) + (y2_-y1_)*(y2_-y1_);
    // If line length ==0, check distance to point
    if (lengthSquared ==0.0) {
        double distance = std::sqrt((x -x1_)* (x-x1_) + (y- y1_)*(y -y1_));
        return distance <= tolerance;
    }
    // Calculate projection of point onto line ;
    double t = ((x -x1_) *(x2_-x1_) + (y -y1_)*(y2_-y1_)) / lengthSquared;
    // Clamp t to [0, 1] to keep the closest point on the segment
    t = std::max(0.0, std::min(1.0, t)); //t represents where the closest point is on the line segment -->t=0 means start point, 
    // Find the closest point on the line segment                                                      --> t=1 means end point
    double closestX = x1_+ t*(x2_- x1_);
    double closestY = y1_ + t*(y2_-y1_);
    // Calculate distance from input point to closest point
    double distX= x-closestX;
    double distY= y-closestY;
    double distance =std::sqrt(distX * distX + distY * distY);
    return distance <=tolerance;
}
/**
 * Helper function to update bounding box from line endpoints.
 * The bounding box is the smallest rectangle that contains the line.
 */
void Line::updateBoundingBox() {
    // Find min and max coordinates for both x and y
    double minX =std::min(x1_,x2_);double minY = std::min(y1_,y2_); double maxX = std::max(x1_,x2_); double maxY = std::max(y1_,y2_);
    // Set bounding box
    setX(minX); setY(minY); setWidth(maxX -minX); setHeight(maxY -minY);
}

// In src/core/Line.cpp

void Line::move(double dx, double dy) {
    // DEBUG PRINT
    // std::cout << "Line is Moving! dx=" << dx << " dy=" << dy << std::endl; 

    x1_ += dx;
    y1_ += dy;
    x2_ += dx;
    y2_ += dy;
    GraphicsObject::move(dx, dy);
}
void Line::scale_factor(double factor) {
    double dx = x2_ - x1_;
    double dy = y2_ - y1_;
    
    x2_=x1_+(dx* factor);
    y2_=y1_ +(dy *factor);
}
// bool Line::contains(double x, double y) const {
//     // ... existing math ...
    
//     // Check if we found it
//     if (distance <= tolerance) {
//         std::cout << "Clicked the Line!" << std::endl; // DEBUG PRINT
//         return true;
//     }
//     return false;
// }