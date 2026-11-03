#include "auth/Credentials.h"

#include <algorithm>

namespace vec {

namespace {

constexpr size_t maxEmailLength = 254;

bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

}  // namespace

std::optional<std::string> normalizeEmail(std::string_view email) {
    while (!email.empty() && isSpace(email.front())) email.remove_prefix(1);
    while (!email.empty() && isSpace(email.back())) email.remove_suffix(1);
    if (email.empty() || email.size() > maxEmailLength) return std::nullopt;

    std::string normalized;
    for (const char c : email) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte <= ' ' || byte == 0x7F) return std::nullopt;  // spaces and control characters
        normalized += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    const size_t at = normalized.find('@');
    if (at == 0 || at == std::string::npos || normalized.find('@', at + 1) != std::string::npos)
        return std::nullopt;
    const std::string_view domain = std::string_view(normalized).substr(at + 1);
    const size_t dot = domain.rfind('.');
    if (dot == std::string_view::npos || dot == 0 || dot == domain.size() - 1) return std::nullopt;
    return normalized;
}

bool isAcceptablePassword(std::string_view password) {
    return password.size() >= minPasswordLength && password.size() <= maxPasswordLength;
}

}  // namespace vec
