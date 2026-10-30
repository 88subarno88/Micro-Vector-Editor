#ifndef RESIZECOMMAND_H
#define RESIZECOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"

class ResizeCommand:public Command {
public:
    // scale -> increase / decrease 
    ResizeCommand(Diagram* diagram, ShapeId id, double scaler);

    void execute() override;
    void undo() override;

private:
    Diagram* diagram;
    ShapeId id;
    double change_factor;
};

#endif
