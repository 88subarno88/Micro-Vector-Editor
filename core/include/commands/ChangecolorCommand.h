#ifndef CHANGECOLORCOMMAND_H
#define CHANGECOLORCOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"
#include <string>

class ChangeColorCommand : public Command {
private:
    Diagram* diagram;
    ShapeId id;
    std::string newColor;
    std::string oldColor;
    bool isFill; // true =change Fill, false =change Stroke

public:
    // Constructor
    ChangeColorCommand(Diagram* diagram, ShapeId id, std::string color, bool changeFill);

    void execute() override;
    void undo() override;
};

#endif
