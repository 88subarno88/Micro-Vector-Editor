#include "commands/CommandStack.h"
#include <iostream>

void CommandStack::execute(std::unique_ptr<Command> cmd) {
    // clear "Redo" history if we are branching off
    if (currentIndex < (int)history.size() - 1) {
        history.erase(history.begin() + currentIndex + 1, history.end());
    }

    
    cmd->execute();

    // add to storage
    history.push_back(std::move(cmd));
    currentIndex++;

    
    // std::cout << "[Stack] Executed. History Size: " << history.size()      // Debug
    //           << " | Current Index: " << currentIndex << std::endl;
}

void CommandStack::undo() {
    // std::cout << "[Stack] Undo Requested. Current Index: " << currentIndex << std::endl;    // Debug

    if (canUndo()) {
        history[currentIndex]->undo();
        currentIndex--;
        // std::cout << "[Stack] Undo Success. New Index: " << currentIndex << std::endl;       // Debug
    } else {
        // std::cout << "[Stack] Cannot Undo (Stack Empty or at Start)." << std::endl;          // Debug
    }
}

void CommandStack::redo() {
    if (canRedo()) {
        currentIndex++;
        history[currentIndex]->execute();
        // std::cout << "[Stack] Redo Success. New Index: " << currentIndex << std::endl;         // Debug
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