#include "core/Rectangle.h"
#include <sstream>

/**
 * Rectangle class 
 * ------- represents a rectangular shape
 * ------- a rectangle is defined by--->a) Top-left corner (x, y)
 *                                  --->b) Width and Height
 */
Rectangle::Rectangle(double x, double y, double width, double height):
     GraphicsObject(x, y, width, height) {}

// Destructor
Rectangle::~Rectangle(){}

// Drawing the rectangle using QPainter
void Rectangle::draw(QPainter* painter) {
    (void)painter; 
}

/**
 * Convert rectangle to SVG format.
 * Returns: <rect x="..." y="..." width="..." height="..." stroke="..." fill="..." />
 */
std::string Rectangle::toSVG() const {
    std::ostringstream svg;
    
    svg << "<rect ";
    svg << "x=\"" << getX() << "\" ";
    svg << "y=\"" << getY() << "\" ";
    svg << "width=\"" << getWidth() << "\" ";
    svg << "height=\"" << getHeight() << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"" << getFillColor() << "\"";
    svg << " />";
    
    return svg.str();
}

// Create the deep copy of the rectangle | used for copy pasting purpose
std::unique_ptr<GraphicsObject> Rectangle::clone() const {
    auto copy = std::make_unique<Rectangle>(getX(), getY(), getWidth(), getHeight());
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
    return copy;
}

// Get type, here "Rectangle"
std::string Rectangle::getType() const {
    return "Rectangle";
}