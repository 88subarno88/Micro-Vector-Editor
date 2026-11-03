#ifndef SERVER_REFRESH_TOKEN_H
#define SERVER_REFRESH_TOKEN_H

#include <chrono>
#include <string>
#include <string_view>

namespace vec::refresh {

constexpr std::chrono::hours lifetime{24 * 30};

struct NewToken {
    std::string token;  // for the client, never stored
    std::string hash;   // for the database
};

// 256 random bits, base64url-encoded, and their hash.
NewToken generate();

// SHA-256 in hex. A fast hash is enough: unlike a password, a 256-bit random token can't be guessed.
std::string hashToken(std::string_view token);

}  // namespace vec::refresh

#endif  // SERVER_REFRESH_TOKEN_H
