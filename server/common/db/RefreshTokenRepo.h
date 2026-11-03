#ifndef SERVER_REFRESH_TOKEN_REPO_H
#define SERVER_REFRESH_TOKEN_REPO_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <string>
#include <tl/expected.hpp>
#include <utility>

namespace vec {

enum class RotateError {
    Invalid,  // unknown, expired, or replaced within the grace period
    Reused,   // replaced or logged out longer ago: probably stolen, so its whole family is now revoked
};

// How long a replaced token is refused without counting as reuse, so that two tabs refreshing at once
// don't log the user out. An attacker replaying within it gets nothing, but goes undetected.
constexpr std::chrono::seconds reuseGracePeriod{10};

// Reads and writes the refresh_tokens table. Tokens are identified by their hash. Unexpected database
// failures throw DrogonDbException.
class RefreshTokenRepo {
public:
    explicit RefreshTokenRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // Stores a token from a new login, in a new family, valid for `lifetime` from now.
    drogon::Task<> create(const std::string& userId, const std::string& tokenHash,
                          std::chrono::seconds lifetime);

    // Replaces `oldHash` with `newHash`, in the same family, valid for `lifetime` from now, and returns the
    // user's ID. If several requests rotate one token at once, exactly one succeeds.
    drogon::Task<tl::expected<std::string, RotateError>> rotate(const std::string& oldHash,
                                                                const std::string& newHash,
                                                                std::chrono::seconds lifetime);

    // Revokes a token, as at logout. False if it was unknown or already revoked.
    drogon::Task<bool> revoke(const std::string& tokenHash);

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_REFRESH_TOKEN_REPO_H
