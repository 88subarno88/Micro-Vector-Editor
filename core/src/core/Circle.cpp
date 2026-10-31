#include "core/Circle.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "core/Renderer.h"

// this is circle class
//  a circle is defined by a centre point(centerX, centerY) and radius
Circle::Circle(double centerX, double centerY, double radius)
    : GraphicsObject(centerX - radius,  // x (top-left of bounding box)
                     centerY - radius,  // y (top-left of bounding box)
                     radius * 2,        // width (diameter)
                     radius * 2         // height(diameter)
                     ),
      center_x_(centerX),
      center_y_(centerY),
      radius_(radius) {}

void Circle::draw(Renderer& renderer) const {
    renderer.drawEllipse({{center_x_, center_y_}, radius_, radius_}, style());
}
/**
 * Convert circle to SVG format.
 * Returns: <circle cx="..." cy="..." r="..." stroke="..." fill="..." />
 */
std::string Circle::toSVG() const {
    std::ostringstream svg;
    svg << "<circle " << idAttribute();
    svg << "cx=\"" << center_x_ << "\" ";
    svg << "cy=\"" << center_y_ << "\" ";
    svg << "r=\"" << radius_ << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"" << getFillColor() << "\"";
    svg << " />";

    return svg.str();
}

// Create the deep copy of the object | used for copy pasting purpose
std::unique_ptr<GraphicsObject> Circle::clone() const {
    auto copy = std::make_unique<Circle>(center_x_, center_y_, radius_);

    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());

    return copy;
}
// Get type,here "Circle"
std::string Circle::getType() const { return "Circle"; }
// check if the point(x,y) is inside the circle
// inside the circle if (x - center_x_)^2+ (y - center_y_)^2<=radius^2
bool Circle::contains(double x, double y) const {
    double dx = x - center_x_;
    double dy = y - center_y_;
    double distance_squared = dx * dx + dy * dy;
    double radius_squared = radius_ * radius_;

    return distance_squared <= radius_squared;
}

// move
void Circle::move(double dx, double dy) {
    center_x_ += dx;
    center_y_ += dy;

    // update the bounding box
    setX(getX() + dx);
    setY(getY() + dy);
}
// for resize
void Circle::scaleBy(double factor) {
    radius_ *= factor;
    // keep the bounding box around the new size
    setPosition(center_x_ - radius_, center_y_ - radius_);
    setWidth(radius_ * 2);
    setHeight(radius_ * 2);
}