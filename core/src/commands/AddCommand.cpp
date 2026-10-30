#include "commands/AddCommand.h"

AddCommand::AddCommand(Diagram* d, std::unique_ptr<GraphicsObject> s)
    : diagram(d), id(s->getId()), shape_ownership(std::move(s)) {
}

void AddCommand::execute() {
    // If we hold the shape , give it to the diagram
    if (shape_ownership) {
        diagram->addObject(std::move(shape_ownership));
    }
}

void AddCommand::undo() {
    // take the shape BACK from the diagram
    shape_ownership = diagram->removeObject(id);
}
