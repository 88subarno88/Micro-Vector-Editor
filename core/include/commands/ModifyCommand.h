#ifndef MODIFYCOMMAND_H
#define MODIFYCOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"
#include <string>

class ModifyCommand :public Command {
public:
    // pass the obj to new settings
    ModifyCommand(Diagram* diagram, ShapeId id, const std::string& newStroke, const std::string& newFill,double newWidth);

    void execute() override;
    void undo() override;

private:
    Diagram* diagram;
    ShapeId id;
    
    //new vals
    std::string new_Stroke, new_Fill;
    double new_Width;

    //old vals
    std::string old_Stroke, old_Fill;
    double old_Width = 0.0;
};

#endif
