#ifndef WANT_TO_INCLUDE_HEXAGON_H
#define WANT_TO_INCLUDE_HEXAGON_H

#include "core/GraphicsObject.h"
#include <cmath>

//A hexagonal shape has been defined by its centre point (centerX, centerY) and radius(distance of centre from any vertex)
class Hexagon : public GraphicsObject {
private:
    double center_x_;    // X coordinate of hexagonal center
    double center_y_;    // Y coordinate of hexagonal center
    double radius_;      // Radius of the hexagon 

public:
    // Constructor
    Hexagon(double centerX=0.0, double centerY=0.0, double radius=10.0);
    
    // Destructor
    virtual ~Hexagon();
    
    // Implement pure virtual functions from GraphicsObject
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
    
    
    //  hit detection (assuming a circle)
    bool contains(double x, double y) const override;
};

#endif // WANT_TO_INCLUDE_HEXAGON_H