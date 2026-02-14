#ifndef FREEHANDSCETCH_H
#define FREEHANDSCETCH_H

#include "core/GraphicsObject.h"
#include <cmath>
#include <vector>
#include <memory>
#include <string>

class QPainter;

struct Point {
    double x;
    double y;
    Point(double xCoord=0.0, double yCoord=0.0) : x(xCoord), y(yCoord) {}
};

class FreehandSketch : public GraphicsObject {
private:    
    std::vector<Point> points_;  // list of points defining the freehand sketch
    void updateBoundingBox();
public:
    FreehandSketch();
    virtual ~FreehandSketch(); 
    
     
    // implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;

    void addPoint(double x, double y);           // add a point to the path
    void clearPoints();                          // clear all points
    int getPointCount() const;                   // how many points?
    const std::vector<Point>& getPoints() const; // get all points

    //do (x,y) in freehandsketch?
    bool contains(double x, double y) const override;
    //resize
    void scale_factor(double factor) override;
};

#endif // FREEHANDSCETCH_H
    
    


