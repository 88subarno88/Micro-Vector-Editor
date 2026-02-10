#ifndef DELETECOMMAND_H
#define DELETECOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"
#include "core/GraphicsObject.h"

class DeleteCommand : public Command {
public:
    DeleteCommand(Diagram* diagram, GraphicsObject* object);

    void execute() override;
    void undo() override;

private:
    Diagram* diagram;
    GraphicsObject* object_Ptr; //helps to find obj
    std::unique_ptr<GraphicsObject> deleted_Shape; //store the deleted shape
};

#endif