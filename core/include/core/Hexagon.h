#ifndef INCLUDE_HEXAGON_H
#define INCLUDE_HEXAGON_H

#include "core/GraphicsObject.h"
#include <cmath>

//A hexagonal shape has been defined by its centre point (centerX, centerY) and radius(distance of centre from any vertex)
class Hexagon : public GraphicsObject {
private:
    double center_x_;    // X coordinate of hexagonal center
    double center_y_;    // Y coordinate of hexagonal center
    double radius_;      // radius of the hexagon 

public:
    Hexagon(double centerX=0.0, double centerY=0.0, double radius=10.0);
    virtual ~Hexagon();
    
    // implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    
    // setters
    void setCenterX(double x);
    void setCenterY(double y);
    void setRadius(double r);
    void setCenter(double x, double y);
    void move(double dx, double dy) override;

     // getters
    double getCenterX() const{return center_x_;}
    double getCenterY() const{return center_y_;}
    double getRadius() const{return radius_;}
    
    
    // do (x,y) inside or on the hexagon?(roughly assuming hexagon as a circle)
    bool contains(double x, double y) const override;
    //resize
    void scale_factor(double factor) override;
};

#endif // HEXAGON_H