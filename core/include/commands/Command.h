#ifndef COMMAND_H
#define COMMAND_H

class Command {
public:
    virtual ~Command() = default;

    // do the action
    virtual void execute() = 0;

    // reverse the action
    virtual void undo() = 0;
};

#endif