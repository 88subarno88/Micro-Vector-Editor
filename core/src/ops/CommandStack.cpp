#include "ops/CommandStack.h"

#include <utility>

namespace vec {

CommandStack::Result CommandStack::execute(Diagram& diagram, const Operation& op) {
    auto inverse = invert(diagram, op);  // needs the diagram as it is before `op`
    if (!inverse) return tl::unexpected(inverse.error());
    if (auto applied = vec::apply(diagram, op); !applied) return applied;

    history_.resize(applied_);
    history_.push_back({op, std::move(*inverse)});
    ++applied_;
    return {};
}

CommandStack::Result CommandStack::undo(Diagram& diagram) {
    if (!canUndo()) return {};
    if (auto applied = vec::apply(diagram, history_[applied_ - 1].inverse); !applied) return applied;
    --applied_;
    return {};
}

CommandStack::Result CommandStack::redo(Diagram& diagram) {
    if (!canRedo()) return {};
    if (auto applied = vec::apply(diagram, history_[applied_].op); !applied) return applied;
    ++applied_;
    return {};
}

void CommandStack::clear() {
    history_.clear();
    applied_ = 0;
}

}  // namespace vec
