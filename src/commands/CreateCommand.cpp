#include "commands/CreateCommand.h"
#include <iostream>

CreateCommand::CreateCommand(Diagram* d, std::unique_ptr<GraphicsObject> s)
    : diagram(d), undone_shape(std::move(s)), shapeaddress_ptr(nullptr) {
}

void CreateCommand::execute() {
    if (!undone_shape) return;

    // 1. SAVE POINTER
    shapeaddress_ptr = undone_shape.get(); 

    // 2. MOVE TO DIAGRAM
    diagram->addObject(std::move(undone_shape));
    
    std::cout << "[Command] Shape Added. Address: " << shapeaddress_ptr << std::endl;
}

void CreateCommand::undo() {
    // Debug: Tell us if the pointer is null!
    if (!shapeaddress_ptr) {
        std::cout << "[Command] ERROR: shapeaddress_ptr is NULL! Cannot Undo." << std::endl;
        return;
    }

    // 3. RETRIEVE FROM DIAGRAM
    undone_shape = diagram->removeObject(shapeaddress_ptr);

    if (undone_shape) {
        std::cout << "[Command] Undo Successful. Shape retrieved." << std::endl;
    } else {
        std::cout << "[Command] CRITICAL FAILURE: Diagram returned nullptr!" << std::endl;
    }
}