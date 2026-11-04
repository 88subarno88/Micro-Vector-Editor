#ifndef SERVER_TITLE_H
#define SERVER_TITLE_H

#include <optional>
#include <string>
#include <string_view>

namespace vec {

constexpr size_t maxTitleLength = 200;  // bytes

// `title` without leading and trailing spaces. nullopt if that's empty, longer than maxTitleLength, or
// contains a control character: NUL can't be stored in Postgres, and line breaks have no place in a title.
std::optional<std::string> normalizeTitle(std::string_view title);

}  // namespace vec

#endif  // SERVER_TITLE_H
