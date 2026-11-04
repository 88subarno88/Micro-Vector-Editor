#ifndef SERVER_ACCESS_H
#define SERVER_ACCESS_H

#include <optional>

#include "db/Models.h"

namespace vec {

enum class Access {
    Allowed,
    NotFound,   // not a member: answer as if the document didn't exist, so its ID reveals nothing
    Forbidden,  // a member, but with too little access
};

// Whether a user with `role` on a document (nullopt if not a member) may do something needing `minimum`.
Access decideAccess(std::optional<Role> role, Role minimum);

}  // namespace vec

#endif  // SERVER_ACCESS_H
