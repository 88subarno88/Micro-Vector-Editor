#ifndef CHANGESTROKEWIDTHCOMMAND_H
#define CHANGESTROKEWIDTHCOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"

class ChangeStrokeWidthCommand : public Command {
public:
    // constructor takes the shape and the  new width 
    ChangeStrokeWidthCommand(GraphicsObject* obj, double new_width);

    void execute() override; // applies the new width 
    void undo() override;    // reverts to the old width

private:
    GraphicsObject* object;
    double old_width;
    double new_width;
};

#endif