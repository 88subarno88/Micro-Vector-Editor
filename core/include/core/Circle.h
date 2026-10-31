#ifndef CIRCLE_H
#define CIRCLE_H

#include <cmath>

#include "core/GraphicsObject.h"

// this is circle class
//  a circle is defined by a centre point(centerX, centerY) and radius
class Circle : public GraphicsObject {
private:
    double center_x_;  // x coordinate of circle center
    double center_y_;  // y coordinate of circle center
    double radius_;    // radius of the circle

public:
    Circle(double centerX = 0.0, double centerY = 0.0, double radius = 10.0);

    // Implement pure virtual functions from GraphicsObject
    void draw(Renderer& renderer) const override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;

    void move(double dx, double dy) override;

    // getters for circle
    double getCenterX() const { return center_x_; }
    double getCenterY() const { return center_y_; }
    double getRadius() const { return radius_; }

    //  do (x,y) inside/on circle?
    bool contains(double x, double y) const override;
    // resize
    void scaleBy(double factor) override;
};

#endif  // CIRCLE_H