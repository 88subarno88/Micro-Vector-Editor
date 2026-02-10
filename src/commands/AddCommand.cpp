#include "commands/AddCommand.h"
#include <iostream>

AddCommand::AddCommand(Diagram* d, std::unique_ptr<GraphicsObject> s)
    : diagram(d), shape_ownership(std::move(s)) {
    // Keep a reference so we can find it later
    raw_Ptr= shape_ownership.get();
}

void AddCommand::execute() {
    // If we hold the shape (First run OR Redo), give it to the diagram
    if (shape_ownership) {
        diagram->addObject(std::move(shape_ownership));
        std::cout << "AddCommand: Added shape to canvas." << std::endl;
    }
}

void AddCommand::undo() {
    if (diagram && raw_Ptr) {
        // Take the shape BACK from the diagram
        shape_ownership = diagram->removeObject(raw_Ptr);
        std::cout << "AddCommand: Undone (Removed shape)." << std::endl;
    }
}