#include <iostream>
#include "core/Diagram.h"
#include <algorithm>

Diagram::Diagram() {
  //autoamtically initaialize an empty vector
}

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
// get the vector objects_ in return by reference
const std::vector<std::unique_ptr<GraphicsObject>>& Diagram::getShapes() const {
    return objects_;
}

//find an object and if find then remove
std::unique_ptr<GraphicsObject> Diagram::removeObject(GraphicsObject* obj) {
    auto it = std::find_if(objects_.begin(), objects_.end(),
        [obj](const std::unique_ptr<GraphicsObject>& s) { 
            return s.get() == obj; 
        });

    if (it != objects_.end()) {
        std::unique_ptr<GraphicsObject> foundShape = std::move(*it);
        objects_.erase(it); 
        // std::cout << "Diagram: Shape found and removed." << std::endl;         //Debug
        return foundShape;
    }
    
    // std::cout << "Diagram: Shape NOT found." << std::endl;           //debug
    return nullptr;
}
