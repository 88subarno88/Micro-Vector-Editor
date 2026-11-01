#include "db/Models.h"

#include <cctype>

namespace vec {

bool isUuid(std::string_view text) {
    constexpr size_t length = 36;
    if (text.size() != length) return false;
    for (size_t i = 0; i < length; ++i) {
        const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? text[i] != '-' : std::isxdigit(static_cast<unsigned char>(text[i])) == 0) return false;
    }
    return true;
}

std::string_view toString(Role role) {
    switch (role) {
        case Role::Viewer:
            return "viewer";
        case Role::Editor:
            return "editor";
        case Role::Owner:
            return "owner";
    }
    return "";
}

std::optional<Role> roleFromString(std::string_view text) {
    for (const Role role : {Role::Viewer, Role::Editor, Role::Owner}) {
        if (text == toString(role)) return role;
    }
    return std::nullopt;
}

}  // namespace vec
