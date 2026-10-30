#include "commands/ChangecolorCommand.h"
#include <utility>

ChangeColorCommand::ChangeColorCommand(Diagram* d, ShapeId shapeId, std::string color, bool changeFill)
    : diagram(d), id(std::move(shapeId)), newColor(std::move(color)), isFill(changeFill) {
}

void ChangeColorCommand::execute() {
    GraphicsObject* object = diagram->find(id);
    if (!object) return;

    // Save old color
    if (isFill) {
        oldColor = object->getFillColor();
        object->setFillColor(newColor);
    } else {
        oldColor = object->getStrokeColor();
        object->setStrokeColor(newColor);
    }
}

void ChangeColorCommand::undo() {
    GraphicsObject* object = diagram->find(id);
    if (!object) return;

    if (isFill) {
        object->setFillColor(oldColor);
    } else {
        object->setStrokeColor(oldColor);
    }
}
