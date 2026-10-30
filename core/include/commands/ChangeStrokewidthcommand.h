#ifndef CHANGESTROKEWIDTHCOMMAND_H
#define CHANGESTROKEWIDTHCOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"

class ChangeStrokeWidthCommand : public Command {
public:
    // constructor takes the shape and the  new width 
    ChangeStrokeWidthCommand(Diagram* diagram, ShapeId id, double new_width);

    void execute() override; // applies the new width 
    void undo() override;    // reverts to the old width

private:
    Diagram* diagram;
    ShapeId id;
    double old_width = 0.0;
    double new_width;
};

#endif
