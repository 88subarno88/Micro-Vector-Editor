#include "commands/ChangecolorCommand.h"
#include <iostream>

ChangeColorCommand::ChangeColorCommand(GraphicsObject* obj, std::string color, bool changeFill)
    : object(obj), newColor(color), isFill(changeFill) {
    
    // Save old color
    if (isFill) {
        oldColor = object->getFillColor();
    } else {
        oldColor = object->getStrokeColor();
    }
}

void ChangeColorCommand::execute() {
    if (isFill) {
        object->setFillColor(newColor);
        // std::cout << "Changed Fill Color to" << newColor << std::endl;    //Debug
    } else {
        object->setStrokeColor(newColor);
        // std::cout << "Changed Stroke Color to" << newColor << std::endl;     //Debug
    }
}

void ChangeColorCommand::undo() {
    if (isFill) {
        object->setFillColor(oldColor);
    } else {
        object->setStrokeColor(oldColor);
    }
}