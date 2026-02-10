#ifndef MOVECOMMAND_H
#define MOVECOMMAND_H

#include "commands/Command.h"
#include "core/GraphicsObject.h"

class MoveCommand :public Command {
public:
    MoveCommand(GraphicsObject* object, double dx, double dy);

    void execute() override;
    void undo() override;

private:
    GraphicsObject* object;
    double dx, dy;
};

#endif