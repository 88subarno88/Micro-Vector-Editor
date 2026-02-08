
#include "core/Diagram.h"
// Constructor implimentation
Diagram::Diagram() {
  //autoamtically initaialize an empty vector
}
//Destructor implimentation 
Diagram::~Diagram() {
    objects_.clear();
}
//adds a object like the shapes circle,hexagon etc to the objects_ vector
void Diagram::addObject(std::unique_ptr<GraphicsObject> obj) {
    objects_.push_back(std::move(obj));
}
// get number of objects in the  objects_ vector
int  Diagram::getObjectCount() const {
    return objects_.size();
}
//clear all objects in objects_ vector
void Diagram::clearallObjects() {
    objects_.clear();
}
// get the vectro objects_ in return by reference
const std::vector<std::unique_ptr<GraphicsObject>>& Diagram::getShapes() const {
    return objects_;
}
