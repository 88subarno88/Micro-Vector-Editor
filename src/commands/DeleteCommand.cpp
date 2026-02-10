#include "commands/DeleteCommand.h"
#include <iostream>

DeleteCommand::DeleteCommand(Diagram* d, GraphicsObject* obj)
    : diagram(d),object_Ptr(obj),deleted_Shape(nullptr) {
}

void DeleteCommand::execute() {
    //remove from diagram and keep   it  in  deleted_shape
    if (!object_Ptr) return;

    // 1. Remove from Diagram and take ownership
    deleted_Shape= diagram->removeObject(object_Ptr);
    if (deleted_Shape) {
        std::cout <<"DeleteCommand is successful and shape is removed" << std::endl;
    } else {
        std::cout << "Error: DeleteCommand isnot successful and the shape not removed"<< std::endl;
    }
}

void DeleteCommand::undo() {
    if (deleted_Shape) {
        //update object_ptr ; mememory address stays theh same
        object_Ptr=deleted_Shape.get(); 
        //put back in diagram 
        diagram->addObject(std::move(deleted_Shape));
        std::cout << "DeleteCommand  is been undone , shape restored " << std::endl;
    }
}