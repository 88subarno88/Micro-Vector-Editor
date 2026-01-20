#ifndef WANT_TO_INCLUDE_RECTANGLE_H
#define WANT_TO_INCLUDE_RECTANGLE_H

#include "core/GraphicsObject.h"
#include <cmath>

/**
 * Rectangle class 
 * ------- represents a rectangular shape
 * ------- a rectangle is defined by--->a) Top-left corner (x, y)
 *                                  --->b) Width and Height
 */

class Rectangle : public GraphicsObject{
 public:
    //Constructor
    Rectangle(double x = 0.0, double y = 0.0, 
              double width = 0.0, double height = 0.0);
    
    // Destructor
    virtual ~Rectangle();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
};


#endif // WANT_TO_INCLUDE_RECTANGLE_H
