#ifndef CIRCLE_H
#define CIRCLE_H

#include "core/GraphicsObject.h"
#include <cmath>

//this is circle class
// a circle is defined by a centre point(centerX, centerY) and radius
class Circle : public GraphicsObject {
private:
    double center_x_;    // x coordinate of circle center
    double center_y_;    // y coordinate of circle center
    double radius_;      // radius of the circle

public:
    Circle(double centerX = 0.0, double centerY = 0.0, double radius = 10.0);
    
    virtual ~Circle();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    
    // setters for circle
    void setCenterX(double x);
    void setCenterY(double y);
    void setRadius(double r);
    void setCenter(double x, double y);
    void move(double dx, double dy) override;
   

     // getters for circle
    double getCenterX() const { return center_x_; }
    double getCenterY() const { return center_y_; }
    double getRadius() const { return radius_; }
    
    
    //  do (x,y) inside/on circle?
    bool contains(double x, double y) const override;
    //resize
     void scale_factor(double factor) override;
};

#endif // CIRCLE_H