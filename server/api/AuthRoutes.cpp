#include "AuthRoutes.h"

#include <drogon/drogon.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "ApiResponse.h"
#include "AuthFilter.h"
#include "RequestLogging.h"
#include "auth/AccessToken.h"
#include "auth/Credentials.h"
#include "auth/Password.h"
#include "auth/RefreshToken.h"
#include "db/RefreshTokenRepo.h"
#include "db/UserRepo.h"
#include "logging/Log.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using nlohmann::json;

namespace {

// Runs `work` on the hashing pool, then carries on on the request's own thread.
template <typename Work>
drogon::Task<std::invoke_result_t<Work>> offLoop(trantor::EventLoopThreadPool& hashing, Work work) {
    std::optional<std::invoke_result_t<Work>> result;
    co_await drogon::queueInLoopCoro(
        hashing.getNextLoop(), [&] { result = work(); }, trantor::EventLoop::getEventLoopOfCurrentThread());
    co_return std::move(*result);
}

constexpr const char* refreshCookieName = "refresh_token";

// The refresh cookie; a max age of 0 deletes it.
drogon::Cookie refreshCookie(const std::string& token, std::chrono::seconds maxAge) {
    drogon::Cookie cookie(refreshCookieName, token);
    cookie.setHttpOnly(true);  // out of reach of page scripts, so XSS can't steal it
    cookie.setSecure(true);
    cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
    cookie.setPath("/api/auth");  // sent only where it's needed
    cookie.setMaxAge(static_cast<int>(maxAge.count()));
    return cookie;
}

constexpr auto refreshLifetime = std::chrono::duration_cast<std::chrono::seconds>(vec::refresh::lifetime);

drogon::HttpResponsePtr tokenResponse(const vec::AccessTokens& tokens, const std::string& userId,
                                      const std::string& refreshToken) {
    const auto response =
        jsonResponse(drogon::k200OK, {{"accessToken", tokens.issue(userId)},
                                      {"tokenType", "Bearer"},
                                      {"expiresIn", vec::AccessTokens::lifetime.count() * 60}});
    response->addCookie(refreshCookie(refreshToken, refreshLifetime));
    return response;
}

drogon::HttpResponsePtr invalidCredentials() {
    // The same answer for an unknown email and a wrong password, so that neither reveals which emails
    // have accounts.
    return errorResponse(drogon::k401Unauthorized, "invalid_credentials", "wrong email or password");
}

// The string at `key`, or nullopt if `body` isn't an object or the value isn't a string.
std::optional<std::string> stringField(const json& body, const char* key) {
    if (!body.is_object()) return std::nullopt;
    const auto it = body.find(key);
    if (it == body.end() || !it->is_string()) return std::nullopt;
    return it->get<std::string>();
}

}  // namespace

void registerAuthRoutes(const drogon::orm::DbClientPtr& database,
                        std::shared_ptr<trantor::EventLoopThreadPool> hashing, vec::AccessTokens tokens) {
    // Checked against when the email is unknown, so that a miss takes as long as a wrong password.
    const auto dummyHash = std::make_shared<const std::string>(vec::password::hash("dummy password"));

    drogon::app().registerHandler(
        "/api/auth/signup",
        [database, hashing](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const json body = json::parse(request->body(), nullptr, false);
            const auto email = stringField(body, "email");
            const auto password = stringField(body, "password");
            if (!email || !password) {
                co_return errorResponse(drogon::k400BadRequest, "bad_request",
                                        "expected a JSON object with string fields email and password");
            }

            const auto normalized = vec::normalizeEmail(*email);
            if (!normalized)
                co_return errorResponse(drogon::k400BadRequest, "invalid_email", "not a valid email");
            if (!vec::isAcceptablePassword(*password)) {
                co_return errorResponse(drogon::k400BadRequest, "invalid_password",
                                        "password must be 8 to 1024 bytes long");
            }

            const std::string hash =
                co_await offLoop(*hashing, [&] { return vec::password::hash(*password); });
            const auto user = co_await vec::UserRepo(database).create(*normalized, hash);
            if (!user) {
                co_return errorResponse(drogon::k409Conflict, "email_taken",
                                        "an account with this email exists");
            }
            co_return jsonResponse(
                drogon::k201Created,
                {{"id", user->id}, {"email", user->email}, {"createdAt", user->createdAt}});
        },
        {drogon::Post, "RateLimitFilter"});

    drogon::app().registerHandler(
        "/api/auth/login",
        [database, hashing, tokens, dummyHash](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const json body = json::parse(request->body(), nullptr, false);
            const auto email = stringField(body, "email");
            const auto password = stringField(body, "password");
            if (!email || !password) {
                co_return errorResponse(drogon::k400BadRequest, "bad_request",
                                        "expected a JSON object with string fields email and password");
            }
            // Too long to have been accepted at signup, so it can't match: no need to hash it.
            if (password->size() > vec::maxPasswordLength) co_return invalidCredentials();

            const auto normalized = vec::normalizeEmail(*email);
            const auto user = normalized ? co_await vec::UserRepo(database).findByEmail(*normalized)
                                         : std::optional<vec::User>();
            const std::string& storedHash = user ? user->passwordHash : *dummyHash;
            const bool matches =
                co_await offLoop(*hashing, [&] { return vec::password::verify(*password, storedHash); });
            if (!user || !matches) co_return invalidCredentials();

            const auto refreshToken = vec::refresh::generate();
            co_await vec::RefreshTokenRepo(database).create(user->id, refreshToken.hash, refreshLifetime);
            co_return tokenResponse(tokens, user->id, refreshToken.token);
        },
        {drogon::Post, "RateLimitFilter"});

    drogon::app().registerHandler(
        "/api/auth/refresh",
        [database, tokens](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const std::string& current = request->getCookie(refreshCookieName);
            const auto next = vec::refresh::generate();
            const auto userId =
                current.empty()
                    ? tl::expected<std::string, vec::RotateError>(tl::unexpect, vec::RotateError::Invalid)
                    : co_await vec::RefreshTokenRepo(database).rotate(vec::refresh::hashToken(current),
                                                                      next.hash, refreshLifetime);
            if (!userId) {
                if (userId.error() == vec::RotateError::Reused) {
                    vec::logWarning("refresh token reused; its login was revoked", requestIdOf(request));
                }
                const auto response =
                    errorResponse(drogon::k401Unauthorized, "invalid_refresh_token", "log in again");
                response->addCookie(refreshCookie("", std::chrono::seconds(0)));
                co_return response;
            }
            co_return tokenResponse(tokens, *userId, next.token);
        },
        {drogon::Post, "RateLimitFilter"});

    drogon::app().registerHandler(
        "/api/auth/logout",
        [database](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const std::string& current = request->getCookie(refreshCookieName);
            if (!current.empty())
                co_await vec::RefreshTokenRepo(database).revoke(vec::refresh::hashToken(current));

            const auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k204NoContent);
            response->addCookie(refreshCookie("", std::chrono::seconds(0)));
            co_return response;
        },
        {drogon::Post, "RateLimitFilter"});

    drogon::app().registerHandler(
        "/api/me",
        [database](HttpRequestPtr request) -> drogon::Task<HttpResponsePtr> {
            const auto user = co_await vec::UserRepo(database).findById(userIdOf(request));
            // A valid token for a user who no longer exists.
            if (!user) co_return errorResponse(drogon::k401Unauthorized, "unauthorized", "no such user");
            co_return jsonResponse(
                drogon::k200OK, {{"id", user->id}, {"email", user->email}, {"createdAt", user->createdAt}});
        },
        {drogon::Get, "AuthFilter"});
}
