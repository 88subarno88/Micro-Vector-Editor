#include "commands/MoveCommand.h"

MoveCommand::MoveCommand(GraphicsObject* obj, double x, double y)
    : object(obj),dx(x),dy(y) {
}

void MoveCommand::execute() {
    if (object) {
        object->move(dx, dy);
    }
}

void MoveCommand::undo() {
    if (object){
        object->move(-dx, -dy);
    }
}