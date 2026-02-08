// RoundedRectangle.h
// A rectangle with rounded corners
// It's just like a normal rectangle but with curved edges

#ifndef ROUNDED_RECTANGLE_H
#define ROUNDED_RECTANGLE_H

#include "core/GraphicsObject.h"
#include <string>
#include <memory>

class QPainter;

/**
 * RoundedRectangle - A rectangle with rounded corners
 * Has same properties as Rectangle plus corner radius
 */
class RoundedRectangle : public GraphicsObject {
private:
    double corner_radius_;  // How curved the corners are
    
public:
    // Constructor
    // x, y = top-left corner position
    // width, height = size of rectangle
    // radius = how round the corners should be
    RoundedRectangle(double x, double y, double width, double height, 
                     double radius);
    
    // Destructor
    virtual ~RoundedRectangle();
    
    // Draw on screen (when Qt is ready)
    virtual void draw(QPainter* painter) override;
    
    // Convert to SVG format
    virtual std::string toSVG() const override;
    
    // Make a copy
    virtual std::unique_ptr<GraphicsObject> clone() const override;
    
    // Get type name
    virtual std::string getType() const override;
    
    // Get corner radius
    double getCornerRadius() const { return corner_radius_; }
    
    // Set corner radius
    void setCornerRadius(double radius);
    void move(double dx, double dy) override;
};

#endif  // ROUNDED_RECTANGLE_H