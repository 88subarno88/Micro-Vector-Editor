#ifndef ADDCOMMAND_H
#define ADDCOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"
#include "core/Diagram.h"
#include <memory>

class AddCommand : public Command {
private:
    Diagram* diagram;
    GraphicsObject* raw_Ptr; // To find the shape later
    
    // This holds the shape when it is NOT on the canvas (Undo state)
    std::unique_ptr<GraphicsObject> shape_ownership; 

public:
    AddCommand(Diagram* d, std::unique_ptr<GraphicsObject> s);
    
    void execute() override;
    void undo() override;
};

#endif