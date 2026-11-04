#include "docs/Title.h"

#include <algorithm>

namespace vec {

std::optional<std::string> normalizeTitle(std::string_view title) {
    while (!title.empty() && title.front() == ' ') title.remove_prefix(1);
    while (!title.empty() && title.back() == ' ') title.remove_suffix(1);
    if (title.empty() || title.size() > maxTitleLength) return std::nullopt;

    const bool hasControl = std::any_of(title.begin(), title.end(), [](char c) {
        const auto byte = static_cast<unsigned char>(c);
        return byte < 0x20 || byte == 0x7F;
    });
    if (hasControl) return std::nullopt;
    return std::string(title);
}

}  // namespace vec
