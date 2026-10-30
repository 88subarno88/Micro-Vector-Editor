#include "core/Diagram.h"

//adds a object like the shapes circle,hexagon etc to the objects_ vector
void Diagram::addObject(std::unique_ptr<GraphicsObject> obj) {
    insertObject(std::move(obj), objects_.size());
}

void Diagram::insertObject(std::unique_ptr<GraphicsObject> obj, size_t position) {
    objects_.insert(objects_.begin() + position, std::move(obj));

    // this shape and every shape after it now sit at a new position
    for (size_t i = position; i < objects_.size(); ++i) {
        index_[objects_[i]->getId()] = i;
    }
}

std::optional<size_t> Diagram::positionOf(const ShapeId& id) const {
    const auto it = index_.find(id);
    if (it == index_.end()) return std::nullopt;
    return it->second;
}
// get number of objects in the  objects_ vector
int  Diagram::getObjectCount() const {
    return objects_.size();
}
//clear all objects in objects_ vector
void Diagram::clearallObjects() {
    objects_.clear();
    index_.clear();
}
GraphicsObject* Diagram::find(const ShapeId& id) const {
    const auto it = index_.find(id);
    return it == index_.end() ? nullptr : objects_[it->second].get();
}

std::unique_ptr<GraphicsObject> Diagram::removeObject(const ShapeId& id) {
    const auto it = index_.find(id);
    if (it == index_.end()) return nullptr;

    const size_t position = it->second;
    auto shape = std::move(objects_[position]);
    objects_.erase(objects_.begin() + position);
    index_.erase(it);

    // every shape after the removed one moved down by one
    for (size_t i = position; i < objects_.size(); ++i) {
        index_[objects_[i]->getId()] = i;
    }
    return shape;
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

