#ifndef COMMANDSTACK_H
#define COMMANDSTACK_H

#include <vector>
#include <memory>
#include "commands/Command.h" 

class CommandStack {
public:
    // run a command and add it to the history
    void execute(std::unique_ptr<Command> cmd);

    // go back 1 step
    void undo();

    // Go forward 1 step
    void redo();

    // UI Helpers 
    bool canUndo() const;
    bool canRedo() const;

    void clear_();

private:
    std::vector<std::unique_ptr<Command>> history;
    int currentIndex = -1; // -1 means nothing has happened till now
};

#endif