#ifndef MODIFYCOMMAND_H
#define MODIFYCOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"
#include <string>

class ModifyCommand :public Command {
public:
    // pass the obj to new settings
    ModifyCommand(GraphicsObject* obj, const std::string& newStroke, const std::string& newFill,double newWidth);

    void execute() override;
    void undo() override;

private:
    GraphicsObject* object;
    
    //new vals
    std::string new_Stroke, new_Fill;
    double new_Width;

    //old vals
    std::string old_Stroke, old_Fill;
    double old_Width;
};

#endif