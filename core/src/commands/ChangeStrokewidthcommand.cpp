#include "commands/ChangeStrokewidthcommand.h"

ChangeStrokeWidthCommand::ChangeStrokeWidthCommand(GraphicsObject* obj, double target_width)
    : object(obj), new_width(target_width) {
    //useful for undo
    if (object) {
        old_width = object->getStrokeWidth(); 
    }
}

void ChangeStrokeWidthCommand::execute() {
    //for redo
    if (object) {
        object->setStrokeWidth(new_width);
    }
}

void ChangeStrokeWidthCommand::undo() {
    // for undo
    if (object) {
        object->setStrokeWidth(old_width);
    }
}