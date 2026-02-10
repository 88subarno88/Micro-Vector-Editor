#ifndef WANT_TO_INCLUDE_FREEHANDSCETCH_H
#define WANT_TO_INCLUDE_FREEHANDSCETCH_H

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
    std::vector<Point> points_;  // List of points defining the freehand sketch
    void updateBoundingBox();
public:
    // Constructor
    FreehandSketch();
    // Destructor
    virtual ~FreehandSketch(); 
    
     
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;

    void addPoint(double x, double y);           // Add a point to the path
    void clearPoints();                          // Clear all points
    int getPointCount() const;                   // How many points?
    const std::vector<Point>& getPoints() const; // Get all points

    bool contains(double x, double y) const override;
     void scale_factor(double factor) override;
};

#endif // WANT_TO_INCLUDE_FREEHANDSCETCH_H
    
    


