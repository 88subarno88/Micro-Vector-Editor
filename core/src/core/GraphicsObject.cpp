#include "core/GraphicsObject.h"
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

namespace {

// Seeded with 128 bits: two clients on the same seed would generate the same ids.
std::mt19937_64 makeGenerator() {
    std::random_device device;
    std::seed_seq seed{device(), device(), device(), device()};
    return std::mt19937_64(seed);
}

}  // namespace

ShapeId GraphicsObject::generateId() {
    thread_local std::mt19937_64 generator = makeGenerator();
    std::ostringstream id;
    id << "s_" << std::hex << std::setfill('0') << std::setw(16) << generator();
    return id.str();
}

GraphicsObject::GraphicsObject(double x, double y, double width, double height):
     id_(generateId()),
     x_(x),
     y_(y), 
     width_(width), 
     height_(height),
     strokecolor_("black"), //default black
     fillcolor_("white"),//default white
     strokewidth_(1.0){} //default 1px

GraphicsObject::~GraphicsObject() {}


//Check if point(x,y) inside the shape
bool GraphicsObject::contains(double x, double y) const {
    return (x >= x_ && x <= x_ + width_ &&
            y >= y_ && y <= y_ + height_);
}

//Move the x->x+dx 
//Move the y->y+dy 
void GraphicsObject::move(double dx, double dy) {
    x_ +=dx;
    y_ +=dy;
}

void GraphicsObject::scale_factor(double factor) {
    // avoid negetives and negligible and play safe 
    if (factor <= 0.0001) return;
    width_ *= factor;
    height_ *= factor;
}