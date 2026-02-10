#ifndef CHANGECOLORCOMMAND_H
#define CHANGECOLORCOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"
#include <string>

class ChangeColorCommand : public Command {
private:
    GraphicsObject* object;
    std::string newColor;
    std::string oldColor;
    bool isFill; // true =Change Fill, false = Change Stroke

public:
    // Constructor
    ChangeColorCommand(GraphicsObject* obj, std::string color, bool changeFill);

    void execute() override;
    void undo() override;
};

#endif