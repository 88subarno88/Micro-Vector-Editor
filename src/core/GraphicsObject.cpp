#include "core/GraphicsObject.h"
#include <cmath>

/**
 * Constructor
 * sets--------->
 * Default stroke --> black, 
 * fill --> white, 
 * stroke width --> 1 px.
 */

GraphicsObject::GraphicsObject(double x, double y, double width, double height):
     x_(x), 
     y_(y), 
     width_(width), 
     height_(height),
     strokecolor_("black"), 
     fillcolor_("white"),
     strokewidth_(1.0), 
     isselected_(false){}
//Destructor
GraphicsObject::~GraphicsObject() {}

//Check if point(x,y) inside the shape
bool GraphicsObject::contains(double x, double y) const {
    return (x >= x_ && x <= x_ + width_ &&
            y >= y_ && y <= y_ + height_);
}

//Move the x->x+dx (by dx pixels along x axis)
//Move the y->y+dy (by dy pixels along y axis)
void GraphicsObject::move(double dx, double dy) {
    x_ += dx;
    y_ += dy;
}