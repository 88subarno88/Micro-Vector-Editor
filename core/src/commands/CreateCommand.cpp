#include "commands/CreateCommand.h"
#include <iostream>

CreateCommand::CreateCommand(Diagram* d, std::unique_ptr<GraphicsObject> s)
    : diagram(d), undone_shape(std::move(s)), shapeaddress_ptr(nullptr) {
}

void CreateCommand::execute() {
    if (!undone_shape) return;

    // save pointer
    shapeaddress_ptr = undone_shape.get(); 

    // move to diagram
    diagram->addObject(std::move(undone_shape));
    
    // std::cout << "[Command] Shape Added. Address: " << shapeaddress_ptr << std::endl;            //Debug
}

void CreateCommand::undo() {
    if (!shapeaddress_ptr) {
        // std::cout << "[Command] ERROR: shapeaddress_ptr is NULL! Cannot Undo." << std::endl;     //Debug
        return;
    }

    // retrive from diagram
    undone_shape = diagram->removeObject(shapeaddress_ptr);

    if (undone_shape) {
        // std::cout << "[Command] Undo Successful. Shape retrieved." << std::endl;                  //Debug
    } else {
        // std::cout << "[Command] CRITICAL FAILURE: Diagram returned nullptr!" << std::endl;        //Debug
    }
}