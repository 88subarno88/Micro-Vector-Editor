#ifndef INCLUDE_RECTANGLE_H
#define INCLUDE_RECTANGLE_H

#include <cmath>

#include "core/GraphicsObject.h"

// Rectangle class
//  a rectangle is defined by top-left corner(x, y),width and height
class Rectangle : public GraphicsObject {
public:
    Rectangle(double x = 0.0, double y = 0.0, double width = 0.0, double height = 0.0);

    // Implement pure virtual functions from GraphicsObject
    void draw(Renderer& renderer) const override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    void scaleBy(double factor) override;
};

#endif  // INCLUDE_RECTANGLE_H
