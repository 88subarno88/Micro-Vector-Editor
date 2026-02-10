#include "core/Rectangle.h"
#include <sstream>


//Qt includes
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPointF>

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
    if (!painter) return;

    // setup for  Stroke -->Pen
    // convert std::string (example "red") to Qt's QColor
    QColor sColor(QString::fromStdString(getStrokeColor()));
    QPen pen(sColor);
    
    // using Qt's setWidthF  precise double-precision width
    pen.setWidthF(getStrokeWidth());
    painter->setPen(pen);

    // setup for Fill -->Brush
    QString fillColorStr = QString::fromStdString(getFillColor());
    painter->setBrush(QBrush(QColor(fillColorStr)));

    // drawingthe Rectangle
    // using Qt's setWidthF  precise double-precision width
    // drawRect(x, y, width, height) of Qt Framework
    painter->drawRect(QRectF(getX(), getY(), getWidth(), getHeight()));
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

void Rectangle::move(double dx, double dy) {
    GraphicsObject::move(dx, dy); 
}

void Rectangle::scale_factor(double factor) {
    setWidth(getWidth() * factor);
    setHeight(getHeight() * factor);
}