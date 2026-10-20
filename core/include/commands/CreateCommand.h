#ifndef CREATECOMMAND_H
#define CREATECOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"
#include "core/GraphicsObject.h"

class CreateCommand : public Command {
public:
    // passed diagram ->the countainer where the shape will be added
    CreateCommand(Diagram* diagram, std::unique_ptr<GraphicsObject> shape);

    void execute() override;
    void undo() override;

private:
    Diagram* diagram;
    std::unique_ptr<GraphicsObject> undone_shape; // store shape after it is undone 
    GraphicsObject* shapeaddress_ptr;             // track shape's addresss
};

#endif