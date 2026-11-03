#ifndef SERVER_ACCESS_TOKEN_H
#define SERVER_ACCESS_TOKEN_H

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

namespace vec {

enum class TokenError { Invalid, Expired };

// The token from an Authorization header value, "Bearer <token>". The scheme is matched in any case, as
// HTTP requires. nullopt for any other scheme or an empty token.
std::optional<std::string_view> bearerToken(std::string_view header);

// Short-lived access tokens: JWTs signed with HS256, whose subject is a user ID.
class AccessTokens {
public:
    static constexpr std::chrono::minutes lifetime{15};

    explicit AccessTokens(std::string secret) : secret_(std::move(secret)) {}

    std::string issue(const std::string& userId,
                      std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const;

    // The user ID if `token` was issued by us and hasn't expired. Any other algorithm, including "none",
    // is Invalid. Never throws.
    tl::expected<std::string, TokenError> verify(
        const std::string& token,
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const;

private:
    std::string secret_;
};

}  // namespace vec

#endif  // SERVER_ACCESS_TOKEN_H
