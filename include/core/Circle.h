#ifndef WANT_TO_INCLUDE_CIRCLE_H
#define WANT_TO_INCLUDE_CIRCLE_H

#include "core/GraphicsObject.h"
#include <cmath>

/**
 * Circle class 
 * ------- represents a circular shape
 * ------- a cirle is defined by--->a)Center point (centerX, centerY)
 *                              --->b)Radius
 */
class Circle : public GraphicsObject {
private:
    double center_x_;    // X coordinate of circle center
    double center_y_;    // Y coordinate of circle center
    double radius_;      // Radius of the circle

public:
    // Constructor
    Circle(double centerX = 0.0, double centerY = 0.0, double radius = 10.0);
    
    // Destructor
    virtual ~Circle();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    
    // Circle-specific setters
    void setCenterX(double x);
    void setCenterY(double y);
    void setRadius(double r);
    void setCenter(double x, double y);

     // Circle-specific getters
    double getCenterX() const { return center_x_; }
    double getCenterY() const { return center_y_; }
    double getRadius() const { return radius_; }
    
    
    //  Precise circular hit detection
    bool contains(double x, double y) const override;
};

#endif // WANT_TO_INCLUDE_CIRCLE_H