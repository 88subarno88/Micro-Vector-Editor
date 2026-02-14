#ifndef INCLUDE_LINE_H
#define INCLUDE_LINE_H

#include "core/GraphicsObject.h"
/**
 * Line class 
 * ------- represents a line segment
 * ------- a line is defined by--->a) Start point (x1, y1)
 *                             --->b) End point (x2, y2)
 */
class Line : public GraphicsObject {
private:
    double x1_;    // X coordinate of start point
    double y1_;    // Y coordinate of start point
    double x2_;    // X coordinate of end point
    double y2_;    // Y coordinate of end point
    void updateBoundingBox();   // calculate bounding box from endpoints

public:
    Line(double x1 =0.0, double y1 = 0.0, 
         double x2 =0.0, double y2 = 0.0);

    virtual ~Line();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    
    // getters for line
    double getX1() const {return x1_;}
    double getY1() const {return y1_;}
    double getX2() const {return x2_;}
    double getY2() const {return y2_;}
    
    // setters for line
    void setX1(double x);
    void setY1(double y);
    void setX2(double x);
    void setY2(double y);
    void setStartPoint(double x, double y);
    void setEndPoint(double x, double y);
    
    // do line contains point(x,y)?
    bool contains(double x, double y) const override;
    //resize
    void scale_factor(double factor) override;
    
};

#endif // INCLUDE_LINE_H