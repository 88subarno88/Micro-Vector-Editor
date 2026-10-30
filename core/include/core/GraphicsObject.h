#ifndef GRAPHICS_OBJECT_H
#define GRAPHICS_OBJECT_H

#include <string>
#include <memory>
#include "core/Geometry.h"


class Renderer;

// Identifies one shape for its whole life, across undo/redo and between clients.
using ShapeId = std::string;


class GraphicsObject{
private:
    ShapeId id_;         // unique id, assigned on construction
    double x_;           // X coordinate of top-left corner
    double y_;           // Y coordinate of top-left corner
    double width_;       // width of bounding box
    double height_;      // height of bounding box

     // visual properties
    std::string strokecolor_;     // border color 
    std::string fillcolor_;       // interior color
    double strokewidth_;          // border thickness in pixels

public:
    GraphicsObject(double x = 0.0, double y = 0.0, 
                   double width = 0.0, double height = 0.0);

    virtual ~GraphicsObject();

    // a copy would share its id with the original; use clone(), which gets a new id
    GraphicsObject(const GraphicsObject&) = delete;
    GraphicsObject& operator=(const GraphicsObject&) = delete;

    // returns a new random id: "s_" + 16 hex digits
    static ShapeId generateId();

    const ShapeId& getId() const { return id_; }

    // draws the shape onto `renderer`
    virtual void draw(Renderer& renderer) const = 0;

    //help to convert the shape to SVG XML format
    virtual std::string toSVG() const = 0;

    //create the deep copy of the object | used for copy pasting purpose 
    virtual std::unique_ptr<GraphicsObject> clone() const = 0;

    //get the type of the shape as string ; eg--> circle ,rectangle
    virtual std::string getType() const = 0;

    // scalefactor
    virtual void scale_factor(double factor)=0;

    //setters
    void setX(double x) { x_ =x; }
    void setY(double y) { y_ =y; }
    void setWidth(double width) { width_ =width; }
    void setHeight(double height) { height_ =height; }
    void setPosition(double x, double y) { x_ = x; y_ = y; }
    void setStrokeColor(const std::string& color) { strokecolor_ =color; }
    void setFillColor(const std::string& color) { fillcolor_ =color; }
    void setStrokeWidth(double width) { strokewidth_ =width; }
    

    //getters
    double getX() const { return x_; }
    double getY() const { return y_; }
    double getWidth() const { return width_; }
    double getHeight() const { return height_; }
    std::string getStrokeColor() const { return strokecolor_; }
    std::string getFillColor() const { return fillcolor_; }
    double getStrokeWidth() const { return strokewidth_; }

    //do point(x,y) inside the shape?
    virtual bool contains(double x, double y) const;

    //move the x->x+dx
    //move the y->y+dy
    virtual void move(double dx, double dy);

protected:
    // stroke, fill and stroke width bundled for a Renderer
    Style style() const { return {strokecolor_, fillcolor_, strokewidth_}; }
};

#endif  //GRAPHICS_OBJECT_H
                   


