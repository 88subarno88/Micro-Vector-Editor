#include "commands/ResizeCommand.h"
#include <utility>

ResizeCommand::ResizeCommand(Diagram* d, ShapeId shapeId, double f)
    : diagram(d), id(std::move(shapeId)), change_factor(f) {
}

void ResizeCommand::execute() {
    if (GraphicsObject* object = diagram->find(id)) object->scale_factor(change_factor);
}

void ResizeCommand::undo() {
    // example calculation -> undo a scale of 2.0, we scale by 0.5 (1/2.0)
    GraphicsObject* object = diagram->find(id);
    if (object && change_factor != 0) object->scale_factor(1.0/change_factor);
}
