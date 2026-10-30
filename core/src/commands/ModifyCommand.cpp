#include "commands/ModifyCommand.h"
#include <utility>

ModifyCommand::ModifyCommand(Diagram* d, ShapeId shapeId,
                             const std::string& newStroke, 
                             const std::string& newFill, 
                             double newWidth)
    : diagram(d), id(std::move(shapeId)), new_Stroke(newStroke), new_Fill(newFill), new_Width(newWidth) {
}
// change to new
void ModifyCommand::execute() {
    if (GraphicsObject* object = diagram->find(id)) {
        // save current state
        old_Stroke=object->getStrokeColor();
        old_Fill=object->getFillColor();
        old_Width=object->getStrokeWidth();

        object->setStrokeColor(new_Stroke);
        object->setFillColor(new_Fill);
        object->setStrokeWidth(new_Width);
    }
}
// back to old
void ModifyCommand::undo() {
    if (GraphicsObject* object = diagram->find(id)) {
        object->setStrokeColor(old_Stroke);
        object->setFillColor(old_Fill);
        object->setStrokeWidth(old_Width);
    }
}
