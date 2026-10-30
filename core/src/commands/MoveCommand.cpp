#include "commands/MoveCommand.h"
#include <utility>

MoveCommand::MoveCommand(Diagram* d, ShapeId shapeId, double x, double y)
    : diagram(d), id(std::move(shapeId)), dx(x), dy(y) {
}

void MoveCommand::execute() {
    if (GraphicsObject* object = diagram->find(id)) {
        object->move(dx, dy);
    }
}

void MoveCommand::undo() {
    if (GraphicsObject* object = diagram->find(id)) {
        object->move(-dx, -dy);
    }
}
