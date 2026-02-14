#ifndef ADDCOMMAND_H
#define ADDCOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"
#include "core/Diagram.h"
#include <memory>

class AddCommand : public Command {
private:
    Diagram* diagram;
    GraphicsObject* raw_Ptr; // will help to find shapes later
    
    // This holds the shape in undo state
    std::unique_ptr<GraphicsObject> shape_ownership; 

public:
    AddCommand(Diagram* d, std::unique_ptr<GraphicsObject> s);
    
    void execute() override;
    void undo() override;
};

#endif