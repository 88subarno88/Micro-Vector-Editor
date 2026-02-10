#ifndef RESIZECOMMAND_H
#define RESIZECOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"

class ResizeCommand:public Command {
public:
    // scale -> increase / decrease 
    ResizeCommand(GraphicsObject* obj,double scaler);

    void execute() override;
    void undo() override;

private:
    GraphicsObject* object;
    double change_factor;
};

#endif