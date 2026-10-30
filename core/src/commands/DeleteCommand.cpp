#include "commands/DeleteCommand.h"
#include <utility>

DeleteCommand::DeleteCommand(Diagram* d, ShapeId shapeId)
    : diagram(d), id(std::move(shapeId)), deleted_Shape(nullptr) {
}

void DeleteCommand::execute() {
    // remove from Diagram and take ownership
    deleted_Shape = diagram->removeObject(id);
}

void DeleteCommand::undo() {
    if (deleted_Shape) {
        //put back in diagram 
        diagram->addObject(std::move(deleted_Shape));
    }
}
