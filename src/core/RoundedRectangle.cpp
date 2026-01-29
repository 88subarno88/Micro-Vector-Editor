// RoundedRectangle.cpp
// Implementation of rounded rectangle

#include "core/RoundedRectangle.h"
#include <sstream>
#include <algorithm>

RoundedRectangle::RoundedRectangle(double x, double y, double width, double height, double radius)
    : GraphicsObject(x, y, width, height),
      corner_radius_(radius) {
    double max_radius = std::min(width, height) / 2.0;
    if (corner_radius_ > max_radius) {
        corner_radius_ = max_radius;
    }
}

RoundedRectangle::~RoundedRectangle() {}

void RoundedRectangle::draw(QPainter* painter) {
    (void)painter;
}

std::string RoundedRectangle::toSVG() const {
    std::ostringstream svg;
    svg << "<rect ";
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

std::unique_ptr<GraphicsObject> RoundedRectangle::clone() const {
    auto copy = std::make_unique<RoundedRectangle>(getX(), getY(), getWidth(), getHeight(), corner_radius_);
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    return copy;
}

std::string RoundedRectangle::getType() const {
    return "RoundedRectangle";
}

void RoundedRectangle::setCornerRadius(double radius) {
    double max_radius = std::min(getWidth(), getHeight()) / 2.0;
    if (radius > max_radius) {
        corner_radius_ = max_radius;
    } else if (radius < 0) {
        corner_radius_ = 0;
    } else {
        corner_radius_ = radius;
    }
}
