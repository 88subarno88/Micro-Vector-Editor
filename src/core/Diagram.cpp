#include <iostream>
#include "core/Diagram.h"
#include <algorithm>
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

// In src/core/Diagram.cpp

std::unique_ptr<GraphicsObject> Diagram::removeObject(GraphicsObject* obj) {
    // DEBUG: Check what we are looking for
    // std::cout << "Diagram: Looking for shape address: " << obj << std::endl;

    // CHANGE 'shapes' TO 'objects_' HERE
    auto it = std::find_if(objects_.begin(), objects_.end(),
        [obj](const std::unique_ptr<GraphicsObject>& s) { 
            return s.get() == obj; 
        });

    // CHANGE 'shapes' TO 'objects_' HERE TOO
    if (it != objects_.end()) {
        std::unique_ptr<GraphicsObject> foundShape = std::move(*it);
        objects_.erase(it); // Remove from the vector
        // std::cout << "Diagram: Shape found and removed." << std::endl;
        return foundShape;
    }
    
    // std::cout << "Diagram: Shape NOT found." << std::endl;
    return nullptr;
}
