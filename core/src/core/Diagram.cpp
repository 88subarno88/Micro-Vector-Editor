#include <iostream>
#include "core/Diagram.h"
#include <algorithm>

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
// standard SVG header, then one line per shape
std::string Diagram::toSVG() const {
    std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"600\" version=\"1.1\">\n";
    for (const auto& obj : objects_) {
        svg += "  " + obj->toSVG() + "\n";
    }
    return svg + "</svg>";
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
