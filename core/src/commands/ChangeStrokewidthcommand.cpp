#include "commands/ChangeStrokewidthcommand.h"
#include <utility>

ChangeStrokeWidthCommand::ChangeStrokeWidthCommand(Diagram* d, ShapeId shapeId, double target_width)
    : diagram(d), id(std::move(shapeId)), new_width(target_width) {
}

void ChangeStrokeWidthCommand::execute() {
    //for redo
    if (GraphicsObject* object = diagram->find(id)) {
        old_width = object->getStrokeWidth(); //useful for undo
        object->setStrokeWidth(new_width);
    }
}

void ChangeStrokeWidthCommand::undo() {
    // for undo
    if (GraphicsObject* object = diagram->find(id)) {
        object->setStrokeWidth(old_width);
    }
}
