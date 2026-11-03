#include "AuthRoutes.h"

#include <drogon/drogon.h>

#include <string>
#include <utility>

#include "ApiResponse.h"
#include "auth/Credentials.h"
#include "auth/Password.h"
#include "db/UserRepo.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using nlohmann::json;

namespace {

// Runs password::hash on the hashing pool, then carries on on the request's own thread.
drogon::Task<std::string> hashOffLoop(trantor::EventLoopThreadPool& hashing, std::string password) {
    std::string hash;
    co_await drogon::queueInLoopCoro(
        hashing.getNextLoop(), [&] { hash = vec::password::hash(password); },
        trantor::EventLoop::getEventLoopOfCurrentThread());
    co_return hash;
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
                        std::shared_ptr<trantor::EventLoopThreadPool> hashing) {
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

            const std::string hash = co_await hashOffLoop(*hashing, *password);
            const auto user = co_await vec::UserRepo(database).create(*normalized, hash);
            if (!user) {
                co_return errorResponse(drogon::k409Conflict, "email_taken",
                                        "an account with this email exists");
            }
            co_return jsonResponse(
                drogon::k201Created,
                {{"id", user->id}, {"email", user->email}, {"createdAt", user->createdAt}});
        },
        {drogon::Post});
}
