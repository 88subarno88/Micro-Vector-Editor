#ifndef ROUNDED_RECTANGLE_H
#define ROUNDED_RECTANGLE_H

#include "core/GraphicsObject.h"
#include <string>
#include <memory>

class QPainter;

//Rounded Rectangle
//a rounded rectangle is defined by top-left corner(x, y) , width and height
class RoundedRectangle : public GraphicsObject {
private:
    double corner_radius_;  // How curved the corners are
    
public:
    RoundedRectangle(double x, double y, double width, double height, 
                     double radius);
    
    virtual ~RoundedRectangle();
    
    // Implement pure virtual functions from GraphicsObject
    virtual void draw(QPainter* painter) override;
    virtual std::string toSVG() const override;
    virtual std::unique_ptr<GraphicsObject> clone() const override;
    virtual std::string getType() const override;
    double getCornerRadius() const { return corner_radius_; }
    void setCornerRadius(double radius);
    void move(double dx, double dy) override;
    void scale_factor(double factor) override;
};

#endif  // ROUNDED_RECTANGLE_H