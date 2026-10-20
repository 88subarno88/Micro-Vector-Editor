#ifndef Diagram_H
#define Diagram_H

#include <core/GraphicsObject.h>
#include <vector>
#include <string>
#include <memory>


class Diagram{
    private :
        std::vector<std::unique_ptr<GraphicsObject>> objects_; // list of graphics objects in the diagram
    public :
        Diagram();
        ~Diagram();

        // add a graphics object to the diagram
        void addObject(std::unique_ptr<GraphicsObject> obj);

        // remove a graphics object from the diagram by index
        void removeObject(size_t index);

        // get all shapes
        const  std::vector<std::unique_ptr<GraphicsObject>>& getShapes () const ;

        // finds a shape by its pointer, removes it from the list, and returns ownership to the caller
        std::unique_ptr<GraphicsObject> removeObject(GraphicsObject* obj);

        // get the number of graphics objects in the diagram
        int getObjectCount() const;  
        
        //clear all objects
        void clearallObjects();     

};

#endif // Diagram_H

