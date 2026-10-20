#ifndef INCLUDE_RECTANGLE_H
#define INCLUDE_RECTANGLE_H

#include "core/GraphicsObject.h"
#include <cmath>


//Rectangle class
// a rectangle is defined by top-left corner(x, y),width and height
class Rectangle : public GraphicsObject{
 public:
    Rectangle(double x = 0.0, double y = 0.0, 
              double width = 0.0, double height = 0.0);
    virtual ~Rectangle();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    void scale_factor(double factor) override;
};


#endif // INCLUDE_RECTANGLE_H
