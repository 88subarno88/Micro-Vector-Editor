#include "commands/ModifyCommand.h"

ModifyCommand::ModifyCommand(GraphicsObject* obj, 
                             const std::string& newStroke, 
                             const std::string& newFill, 
                             double newWidth)
    : object(obj), new_Stroke(newStroke), new_Fill(newFill), new_Width(newWidth) {
    
    // save current state
    if (object) {
        old_Stroke=object->getStrokeColor();
        old_Fill=object->getFillColor();
        old_Width=object->getStrokeWidth();
    }
}
// change to new
void ModifyCommand::execute() {
    if (object) {
        object->setStrokeColor(new_Stroke);
        object->setFillColor(new_Fill);
        object->setStrokeWidth(new_Width);
    }
}
// back to old
void ModifyCommand::undo() {
    if (object) {
        object->setStrokeColor(old_Stroke);
        object->setFillColor(old_Fill);
        object->setStrokeWidth(old_Width);
    }
}