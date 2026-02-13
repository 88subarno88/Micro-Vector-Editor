#include "commands/CommandStack.h"
#include <iostream>

void CommandStack::execute(std::unique_ptr<Command> cmd) {
    // 1. Clear "Redo" history if we are branching off
    if (currentIndex < (int)history.size() - 1) {
        history.erase(history.begin() + currentIndex + 1, history.end());
    }

    // 2. Run the command
    cmd->execute();

    // 3. Add to storage
    history.push_back(std::move(cmd));
    currentIndex++;

    // DEBUG PRINT
    std::cout << "[Stack] Executed. History Size: " << history.size() 
              << " | Current Index: " << currentIndex << std::endl;
}

void CommandStack::undo() {
    std::cout << "[Stack] Undo Requested. Current Index: " << currentIndex << std::endl;

    if (canUndo()) {
        history[currentIndex]->undo();
        currentIndex--;
        std::cout << "[Stack] Undo Success. New Index: " << currentIndex << std::endl;
    } else {
        std::cout << "[Stack] Cannot Undo (Stack Empty or at Start)." << std::endl;
    }
}

void CommandStack::redo() {
    if (canRedo()) {
        currentIndex++;
        history[currentIndex]->execute();
        std::cout << "[Stack] Redo Success. New Index: " << currentIndex << std::endl;
    }
}

bool CommandStack::canUndo() const {
    return currentIndex >= 0;
}

bool CommandStack::canRedo() const {
    return currentIndex < (int)history.size() - 1;
}

void CommandStack::clear_() {
    // delete all stored commands
    history.clear();
    
    // reset the currentIndex to -1  i.e. the starting state
    currentIndex = -1;
}