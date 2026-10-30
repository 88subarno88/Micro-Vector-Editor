#ifndef COMMANDSTACK_H
#define COMMANDSTACK_H

#include <vector>

#include "ops/Apply.h"

namespace vec {

// Undo/redo history of the operations applied to a diagram.
class CommandStack {
public:
    using Result = tl::expected<void, ApplyError>;

    // applies `op` and adds it to the history, dropping anything that could be redone;
    // an op that fails isn't recorded
    Result execute(Diagram& diagram, const Operation& op);

    // go back 1 step; does nothing if there is nothing to undo
    Result undo(Diagram& diagram);

    // go forward 1 step; does nothing if there is nothing to redo
    Result redo(Diagram& diagram);

    bool canUndo() const { return applied_ > 0; }
    bool canRedo() const { return applied_ < history_.size(); }

    void clear();

private:
    struct Entry {
        Operation op;
        Operation inverse;
    };

    std::vector<Entry> history_;
    size_t applied_ = 0;  // how many entries of history_ are currently applied
};

}  // namespace vec

#endif // COMMANDSTACK_H
