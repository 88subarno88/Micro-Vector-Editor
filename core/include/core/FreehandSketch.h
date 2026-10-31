#ifndef FREEHAND_SKETCH_H
#define FREEHAND_SKETCH_H

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "core/Geometry.h"
#include "core/GraphicsObject.h"

class FreehandSketch : public GraphicsObject {
private:
    std::vector<Point> points_;  // list of points defining the freehand sketch
    void updateBoundingBox();

public:
    FreehandSketch();

    // implement pure virtual functions from GraphicsObject
    void draw(Renderer& renderer) const override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;

    void addPoint(double x, double y);            // add a point to the path
    const std::vector<Point>& getPoints() const;  // get all points

    // do (x,y) in freehandsketch?
    bool contains(double x, double y) const override;
    // resize
    void scaleBy(double factor) override;
};

#endif  // FREEHAND_SKETCH_H
