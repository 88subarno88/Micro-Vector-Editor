#ifndef MOVECOMMAND_H
#define MOVECOMMAND_H

#include "commands/Command.h"
#include "core/Diagram.h"

class MoveCommand :public Command {
public:
    MoveCommand(Diagram* diagram, ShapeId id, double dx, double dy);

    void execute() override;
    void undo() override;

private:
    Diagram* diagram;
    ShapeId id;
    double dx, dy;
};

#endif
