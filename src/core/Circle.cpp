#include "core/Circle.h"
#include <sstream>
#include <iomanip>
#include <cmath>
/**
 * Circle class 
 * ------- represents a circular shape
 * ------- a cirle is defined by--->a)Center point (centerX, centerY)
 *                              --->b)Radius
 */
Circle::Circle(double centerX, double centerY, double radius)
    : GraphicsObject(
        centerX - radius,           // x (top-left of bounding box)
        centerY - radius,           // y (top-left of bounding box)
        radius * 2,                 // width (diameter)
        radius * 2                  // height (diameter)
      ),
      center_x_(centerX),
      center_y_(centerY),
      radius_(radius) {}
// Destructor
Circle::~Circle() {}
// Drawing the circle using QPainter.
void Circle::draw(QPainter* painter) {
    (void)painter;  // Suppress unused parameter warning
}
/**
 * Convert circle to SVG format.
 * Returns: <circle cx="..." cy="..." r="..." stroke="..." fill="..." />
 */
std::string Circle::toSVG() const {
    std::ostringstream svg;  
    svg << "<circle ";
    svg << "cx=\"" << center_x_ << "\" ";
    svg << "cy=\"" << center_y_ << "\" ";
    svg << "r=\"" << radius_ << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"" << getFillColor() << "\"";
    svg << " />";
    
    return svg.str();
}
//Create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> Circle::clone() const {
    auto copy = std::make_unique<Circle>(center_x_, center_y_, radius_);
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
    return copy;
}
//Get type,here "Circle"
std::string Circle::getType() const {
    return "Circle";
}
//set x cordinate of the center of the circle
void Circle::setCenterX(double x) {
    center_x_ = x;
    setX(x - radius_);  // Update bounding box
}
//set y cordinate of the center of the circle
void Circle::setCenterY(double y) {
    center_y_ = y;
    setY(y - radius_);  // Update bounding box
}
//Set radius of the circle and accordingly updating boundry
void Circle::setRadius(double r) {
    radius_ = r;
    setX(center_x_ - r);        // Update bounding box x
    setY(center_y_ - r);        // Update bounding box y
    setWidth(r * 2);            // Update width
    setHeight(r * 2);           // Update height
}
/**
 * Set the center of the circle (both x and y).
 */
void Circle::setCenter(double x, double y) {
    center_x_ = x;
    center_y_ = y;
    setX(x - radius_);
    setY(y - radius_);
}
/**
 * check if the point(x,y) is inside the circle
 * inside the circle---> if (x - center_x_)^2+ (y - center_y_)^2<=radius^2
 */
bool Circle::contains(double x, double y) const {
    double dx = x - center_x_;
    double dy = y - center_y_;
    double distance_squared = dx * dx + dy * dy;
    double radius_squared = radius_ * radius_;
    
    return distance_squared <= radius_squared;
}