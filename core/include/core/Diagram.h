#ifndef Diagram_H
#define Diagram_H

#include <core/GraphicsObject.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>


class Diagram{
    private :
        std::vector<std::unique_ptr<GraphicsObject>> objects_; // list of graphics objects in the diagram
        std::unordered_map<ShapeId, size_t> index_;            // each shape's position in objects_
    public :
        // add a graphics object to the diagram
        void addObject(std::unique_ptr<GraphicsObject> obj);

        // finds a shape by id; nullptr if it isn't in the diagram
        GraphicsObject* find(const ShapeId& id) const;

        // removes a shape by id and returns ownership to the caller; nullptr if it isn't in the diagram
        std::unique_ptr<GraphicsObject> removeObject(const ShapeId& id);

        // get all shapes, in drawing order (first is at the back)
        const  std::vector<std::unique_ptr<GraphicsObject>>& getShapes () const ;

        // get the number of graphics objects in the diagram
        int getObjectCount() const;  
        
        //clear all objects
        void clearallObjects();

        // the whole drawing as an SVG document
        std::string toSVG() const;

};

#endif // Diagram_H

