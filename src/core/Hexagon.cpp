#include "core/Hexagon.h"
#include <sstream>
#include <iomanip>
#include <cmath>
/**
 * Hexagon class 
 * ------- represents a Hexagonal shape
 * ------- a hexagon is defined by--->a)Center point (centerX, centerY)
 *                              --->b)Radius
 */

const double PI=3.14159265358979323846;
Hexagon::Hexagon(double centerX, double centerY, double radius)
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
Hexagon::~Hexagon() {}
// Drawing the hexagon using QPainter.
void Hexagon::draw(QPainter* painter) {
    (void)painter;  // Suppress unused parameter warning
}
/**
 * Convert hexagon to SVG format.
 * Returns: <polygon points="x1,y1 x2,y2 x3,y3 x4,y4 x5,y5 x6,y6" ... />
 */
std::string Hexagon::toSVG() const {
    std::ostringstream svg;  
    svg << "<polygon points=\"";

    for (int i=0;i<6;i++){
        double angle=PI/3*i; //60 degree in radian
        double x= center_x_ + radius_ * cos(angle);
        double y= center_y_ + radius_ * sin(angle);
        svg << x << "," << y;
        if(i!=5) svg << " ";

    }
    svg << "\" "; 
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"" << getFillColor() << "\"";
    svg << " />";
    return svg.str();
}
//Create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> Hexagon::clone() const {
    auto copy = std::make_unique<Hexagon>(center_x_, center_y_, radius_);
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
    return copy;
}
//Get type,here "Hexagon"
std::string Hexagon::getType() const {
    return "Hexagon";
}
//set x cordinate of the center of the hexagon
void Hexagon::setCenterX(double x) {
    center_x_ = x;
    setX(x - radius_);  // Update bounding box
}
//set y cordinate of the center of the hexagon
void Hexagon::setCenterY(double y) {
    center_y_ = y;
    setY(y - radius_);  // Update bounding box
}
//Set radius of the hexagon and accordingly updating boundry
void Hexagon::setRadius(double r) {
    radius_ = r;
    setX(center_x_ - r);        // Update bounding box x
    setY(center_y_ - r);        // Update bounding box y
    setWidth(r * 2);            // Update width
    setHeight(r * 2);           // Update height
}
/**
 * Set the center of the hexagon (both x and y).
 */
void Hexagon::setCenter(double x, double y) {
    center_x_ = x;
    center_y_ = y;
    setX(x - radius_);
    setY(y - radius_);
}
/**
 * check if the point(x,y) is inside the hexagon
 * here I am approximating hexagon as circle for hit detection
 * I think it doesnot ruin user experience much
 * inside the hexagon---> if (x - center_x_)^2+ (y - center_y_)^2<=radius^2
 */
bool Hexagon::contains(double x, double y) const {
    double dx = x - center_x_;
    double dy = y - center_y_;
    double distance_squared = dx * dx + dy * dy;
    double radius_squared = radius_ * radius_;
    
    return distance_squared <= radius_squared;
}