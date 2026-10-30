#ifndef APPLY_H
#define APPLY_H

#include <tl/expected.hpp>

#include "core/Diagram.h"
#include "ops/Operation.h"

namespace vec {

enum class ApplyError {
    ShapeNotFound,  // the op names an id that isn't in the diagram
    DuplicateId,    // AddShape with an id already in the diagram
    InvalidShape,   // AddShape with unusable data, e.g. a font size of 0 or a quote in a colour
    InvalidValue,   // Move, Resize or SetStyle with a number or colour that can't be used
};

// Applies `op` to `diagram`. On an error the diagram is left unchanged.
tl::expected<void, ApplyError> apply(Diagram& diagram, const Operation& op);

// Returns the op that undoes `op`. Call it before applying `op`, while `diagram` still holds
// what the inverse has to restore. A resize may come back off by floating-point rounding.
tl::expected<Operation, ApplyError> invert(const Diagram& diagram, const Operation& op);

}  // namespace vec

#endif // APPLY_H
