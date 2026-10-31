
#include "core/RoundedRectangle.h"

#include <algorithm>
#include <sstream>

#include "core/Renderer.h"

// Rounded Rectangle
// a rounded rectangle is defined by top-left corner(x, y) , width and height
RoundedRectangle::RoundedRectangle(double x, double y, double width, double height, double radius)
    : GraphicsObject(x, y, width, height), corner_radius_(radius) {
    double max_radius = std::min(width, height) / 2.0;

    if (corner_radius_ > max_radius) {
        corner_radius_ = max_radius;
    }
}

void RoundedRectangle::draw(Renderer& renderer) const {
    renderer.drawRect({getX(), getY(), getWidth(), getHeight(), corner_radius_}, style());
}
/**
 * Convert rounded rectangle to SVG format.
 * Returns: <rect x="..." y="..." width="..." height="..." rx="..." ry="..." stroke="..." fill="..." />
 */
std::string RoundedRectangle::toSVG() const {
    std::ostringstream svg;
    svg << "<rect " << idAttribute();
    svg << "x=\"" << getX() << "\" ";
    svg << "y=\"" << getY() << "\" ";
    svg << "width=\"" << getWidth() << "\" ";
    svg << "height=\"" << getHeight() << "\" ";
    svg << "rx=\"" << corner_radius_ << "\" ";
    svg << "ry=\"" << corner_radius_ << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"" << getFillColor() << "\"";
    svg << " />";
    return svg.str();
}

// Create the deep copy of the rounded rectangle | used for copy pasting purpose
std::unique_ptr<GraphicsObject> RoundedRectangle::clone() const {
    auto copy = std::make_unique<RoundedRectangle>(getX(), getY(), getWidth(), getHeight(), corner_radius_);
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    return copy;
}

// get type, here "RoundedRectangle"
std::string RoundedRectangle::getType() const { return "RoundedRectangle"; }

// resize
void RoundedRectangle::scaleBy(double factor) {
    setWidth(getWidth() * factor);
    setHeight(getHeight() * factor);
    corner_radius_ *= factor;
}