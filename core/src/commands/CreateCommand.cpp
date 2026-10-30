#include "commands/CreateCommand.h"

CreateCommand::CreateCommand(Diagram* d, std::unique_ptr<GraphicsObject> s)
    : diagram(d), id(s->getId()), undone_shape(std::move(s)) {
}

void CreateCommand::execute() {
    if (!undone_shape) return;

    // move to diagram
    diagram->addObject(std::move(undone_shape));
}

void CreateCommand::undo() {
    // retrive from diagram
    undone_shape = diagram->removeObject(id);
}
