#include "auth/Access.h"

namespace vec {

Access decideAccess(std::optional<Role> role, Role minimum) {
    if (!role) return Access::NotFound;
    return *role >= minimum ? Access::Allowed : Access::Forbidden;
}

}  // namespace vec
