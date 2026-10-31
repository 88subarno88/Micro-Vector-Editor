#ifndef ROUNDED_RECTANGLE_H
#define ROUNDED_RECTANGLE_H

#include <memory>
#include <string>

#include "core/GraphicsObject.h"

// Rounded Rectangle
// a rounded rectangle is defined by top-left corner(x, y) , width and height
class RoundedRectangle : public GraphicsObject {
private:
    double corner_radius_;  // How curved the corners are

public:
    RoundedRectangle(double x, double y, double width, double height, double radius);

    // Implement pure virtual functions from GraphicsObject
    void draw(Renderer& renderer) const override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    double getCornerRadius() const { return corner_radius_; }
    void scaleBy(double factor) override;
};

#endif  // ROUNDED_RECTANGLE_H