#include "core/FreehandSketch.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

//Qt includes
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPointF>

FreehandSketch::FreehandSketch(): GraphicsObject(0,0,0,0 ){}      // (x,y,width,height)
      
FreehandSketch::~FreehandSketch() {}

// Drawing the freehandsketch using QPainter.
void FreehandSketch::draw(QPainter* painter) {
    if (!painter || points_.empty()) {
        return;
    }

    // adding pen for freehandsketch
    QPen pen{QColor(QString::fromStdString(getStrokeColor()))};
    pen.setWidthF(getStrokeWidth());
    pen.setJoinStyle(Qt::MiterJoin);        //remove rounded corners as width increases
    painter->setPen(pen);
    
    painter->setBrush(Qt::NoBrush); // transparent fill

    // converting points to Qt points
    QVector<QPointF> qtPoints;
    qtPoints.reserve(points_.size());
    
    for (const auto& p : points_) {
        qtPoints.append(QPointF(p.x, p.y));
    }
    // drawing the lines
    painter->drawPolyline(qtPoints);
}

std::string FreehandSketch::toSVG() const {  
    if (points_.empty()) { return "";}
    std::ostringstream svg;

    svg << "<path d=\"";
    svg << "M " << points_[0].x << " " << points_[0].y;
    for (size_t i = 1; i < points_.size(); i++) {
        svg << " L " << points_[i].x << " " << points_[i].y;    //get all pts
    }
    svg << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\" ";
    svg << "fill=\"none\"";  // freehand has none to fill in
    svg << " />";

    return svg.str();
}

//create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> FreehandSketch::clone() const {
    auto copy= std::make_unique<FreehandSketch>();

    // copy all points
    for (const Point& p : points_) {
        copy->addPoint(p.x, p.y);
    }
    
    // copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
    return copy;
}

//get type,here "FreehandSketch"
std::string FreehandSketch::getType() const {
    return "FreehandSketch";
}

// add a point to the freehand sketch
void FreehandSketch::addPoint(double x, double y) {
    points_.push_back(Point(x,y));
    updateBoundingBox();
}

// clear all points in the freehand sketch
void FreehandSketch::clearPoints() {
    points_.clear();

    // Reset bounding box
    setX(0);
    setY(0);
    setWidth(0);
    setHeight(0);
}
// get the total number of points in the freehand sketch
int FreehandSketch::getPointCount() const {
    return points_.size();
}

// get all points_ in the freehand sketch
const std::vector<Point>& FreehandSketch::getPoints() const {
    return points_;
}
//answers(T/F) if we have (x,y) in our freehand sketch
bool FreehandSketch::contains(double x, double y) const {
    return (x>= getX() && x <= getX() + getWidth() && y >= getY() && y <= getY() + getHeight());
}

void FreehandSketch::updateBoundingBox() {
    if (points_.empty()) {
        return;
    }

    double minX=points_[0].x;
    double maxX=points_[0].x;
    double minY=points_[0].y;
    double maxY=points_[0].y;

    for (const Point& p : points_) {
        if (p.x<minX) minX=p.x;
        if (p.x>maxX) maxX=p.x;
        if (p.y<minY) minY=p.y;
        if (p.y>maxY) maxY=p.y;
    }

    setX(minX);
    setY(minY);
    setWidth(maxX-minX);
    setHeight(maxY-minY);
}

//move
void FreehandSketch::move(double dx, double dy) {
    for (auto& point : points_) {
        point.x +=dx; 
        point.y +=dy; 
    }
    //update the box
    GraphicsObject::move(dx,dy);
}
//resize
void FreehandSketch::scale_factor(double factor) {
    if (points_.empty()) return;

    //use first point as pivot
    double pvt_X =points_[0].x;
    double pvt_Y =points_[0].y;

    // iter through points
    for (auto& p : points_) {
        double dx= p.x -pvt_X;
        double dy = p.y -pvt_Y;
        p.x = pvt_X +(dx * factor);
        p.y =pvt_Y + (dy * factor);
    }
    updateBoundingBox();
}