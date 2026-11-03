#include "auth/AccessToken.h"

#include <jwt-cpp/traits/nlohmann-json/defaults.h>

#include <exception>

namespace vec {

namespace {

constexpr const char* issuer = "micro-vector-editor";

// A clock fixed at `now`, so that tests can control expiry.
struct FixedClock {
    std::chrono::system_clock::time_point time;
    std::chrono::system_clock::time_point now() const { return time; }
};

}  // namespace

std::string AccessTokens::issue(const std::string& userId, std::chrono::system_clock::time_point now) const {
    return jwt::create()
        .set_type("JWT")
        .set_issuer(issuer)
        .set_subject(userId)
        .set_issued_at(now)
        .set_expires_at(now + lifetime)
        .sign(jwt::algorithm::hs256{secret_});
}

tl::expected<std::string, TokenError> AccessTokens::verify(const std::string& token,
                                                           std::chrono::system_clock::time_point now) const {
    try {
        const auto decoded = jwt::decode(token);
        if (!decoded.has_subject() || decoded.get_subject().empty() || !decoded.has_expires_at()) {
            return tl::unexpected(TokenError::Invalid);
        }

        std::error_code error;
        jwt::verify<FixedClock, jwt::traits::nlohmann_json>(FixedClock{now})
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .with_issuer(issuer)
            .verify(decoded, error);
        if (error == jwt::error::token_verification_error::token_expired) {
            return tl::unexpected(TokenError::Expired);
        }
        if (error) return tl::unexpected(TokenError::Invalid);
        return decoded.get_subject();
    } catch (const std::exception&) {  // malformed: not three parts, bad base64 or bad JSON
        return tl::unexpected(TokenError::Invalid);
    }
}

}  // namespace vec
