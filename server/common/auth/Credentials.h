#ifndef SERVER_CREDENTIALS_H
#define SERVER_CREDENTIALS_H

#include <optional>
#include <string>
#include <string_view>

namespace vec {

// Passwords are bytes. Long enough to resist guessing; capped because hashing reads every byte.
constexpr size_t minPasswordLength = 8;
constexpr size_t maxPasswordLength = 1024;

// `email` trimmed and with ASCII letters lowercased, so that "Ann@X.com " and "ann@x.com" are one
// account. nullopt unless it looks like local@domain.tld: one '@', no spaces or control characters,
// and at most 254 bytes. Deliverability is not checked.
std::optional<std::string> normalizeEmail(std::string_view email);

bool isAcceptablePassword(std::string_view password);

}  // namespace vec

#endif  // SERVER_CREDENTIALS_H
