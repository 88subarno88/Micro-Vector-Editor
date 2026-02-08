#ifndef WANT_TO_INCLUDE_GRAPHICS_OBJECT_H
#define WANT_TO_INCLUDE_GRAPHICS_OBJECT_H

#include <string>
#include <memory>
#include <QPainter>


class QPainter;


class GraphicsObject{
private:
  // Position and dimensions
    double x_;           // X coordinate of top-left corner
    double y_;           // Y coordinate of top-left corner
    double width_;       // Width of bounding box
    double height_;      // Height of bounding box

     // Visual properties
    std::string strokecolor_;     // Border color (e.g., "red","#FF0000")
    std::string fillcolor_;       // Interior color
    double strokewidth_;          // Border thickness in pixels
    
    // Selection state
    bool isselected_;

public:
    //Constructor with default values
    GraphicsObject(double x = 0.0, double y = 0.0, 
                   double width = 0.0, double height = 0.0);

    //Destructor 
    virtual ~GraphicsObject();

    //Draw the shape on the canvas using QPainter
    virtual void draw(QPainter* painter) = 0;

    //Help to convert the shape to SVG XML format
    virtual std::string toSVG() const = 0;

    //Create the deep copy of the object | used for copy pasting purpose 
    virtual std::unique_ptr<GraphicsObject> clone() const = 0;

    //Get the type of the shape as string ; eg--> circle ,rectangle
    virtual std::string getType() const = 0;

    //SETTERS 
    void setX(double x) { x_ = x; }
    void setY(double y) { y_ = y; }
    void setWidth(double width) { width_ = width; }
    void setHeight(double height) { height_ = height; }
    void setPosition(double x, double y) { x_ = x; y_ = y; }
    void setStrokeColor(const std::string& color) { strokecolor_ = color; }
    void setFillColor(const std::string& color) { fillcolor_ = color; }
    void setStrokeWidth(double width) { strokewidth_ = width; }
    void setSelected(bool selected) { isselected_ = selected; }
    

    //GETTERS
    double getX() const { return x_; }
    double getY() const { return y_; }
    double getWidth() const { return width_; }
    double getHeight() const { return height_; }
    std::string getStrokeColor() const { return strokecolor_; }
    std::string getFillColor() const { return fillcolor_; }
    double getStrokeWidth() const { return strokewidth_; }
    bool isSelected() const { return isselected_; }

    //Check if point(x,y) inside the shape
    virtual bool contains(double x, double y) const;

    //Move the x->x+dx (by dx pixels along x axis)
    //Move the y->y+dy (by dy pixels along y axis)
    virtual void move(double dx, double dy);
};

#endif  //WANT_TO_INCLUDE_GRAPHICS_OBJECT_H
                   


