#ifndef Diagram_H
#define Diagram_H

#include <core/GraphicsObject.h>
#include <vector>
#include <string>
#include <memory>


class Diagram{
    private :
        std::vector<std::unique_ptr<GraphicsObject>> objects_; // List of graphics objects in the diagram
    public :
        // Constructor
        Diagram();
        // Destructor
        ~Diagram();
        // Add a graphics object to the diagram
        void addObject(std::unique_ptr<GraphicsObject> obj);
        // Remove a graphics object from the diagram by index
        void removeObject(size_t index);
        // get all shapes
        const  std::vector<std::unique_ptr<GraphicsObject>>& getShapes () const ;
         // get the number of graphics objects in the diagram
        int getObjectCount() const;  
        //clear all objects
        void clearallObjects();     

};

#endif // Diagram_H

