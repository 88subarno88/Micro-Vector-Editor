#include "commands/ResizeCommand.h"

ResizeCommand::ResizeCommand(GraphicsObject* obj, double f)
    : object(obj), change_factor(f) {
}

void ResizeCommand::execute() {
    if (object) object->scale_factor(change_factor);
}

void ResizeCommand::undo() {
    // example calculation -> undo a scale of 2.0, we scale by 0.5 (1/2.0)
    if (object && change_factor != 0) object->scale_factor(1.0/change_factor);
}