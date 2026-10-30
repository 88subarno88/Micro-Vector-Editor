#ifndef APPLY_H
#define APPLY_H

#include <tl/expected.hpp>

#include "core/Diagram.h"
#include "ops/Operation.h"

namespace vec {

enum class ApplyError {
    ShapeNotFound,  // the op names an id that isn't in the diagram
    DuplicateId,    // AddShape with an id already in the diagram
    InvalidShape,   // AddShape with data no shape can have, e.g. a font size of 0
    Unsupported,    // this op can't be applied yet
};

// Applies `op` to `diagram`. On an error the diagram is left unchanged.
tl::expected<void, ApplyError> apply(Diagram& diagram, const Operation& op);

}  // namespace vec

#endif // APPLY_H
