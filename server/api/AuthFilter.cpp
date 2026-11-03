#include "AuthFilter.h"

#include "ApiResponse.h"
#include "db/Models.h"

namespace {

constexpr const char* userIdAttribute = "userId";

drogon::HttpResponsePtr unauthorized(const std::string& code, const std::string& message) {
    const auto response = errorResponse(drogon::k401Unauthorized, code, message);
    response->addHeader("WWW-Authenticate", "Bearer");
    return response;
}

}  // namespace

void AuthFilter::doFilter(const drogon::HttpRequestPtr& request, drogon::FilterCallback&& reject,
                          drogon::FilterChainCallback&& pass) {
    const auto token = vec::bearerToken(request->getHeader("authorization"));
    if (!token) return reject(unauthorized("unauthorized", "send an access token as Authorization: Bearer"));

    const auto userId = tokens_.verify(std::string(*token));
    if (!userId) {
        if (userId.error() == vec::TokenError::Expired) {
            return reject(unauthorized("token_expired", "the access token has expired; refresh it"));
        }
        return reject(unauthorized("unauthorized", "the access token is not valid"));
    }
    // Only this server can sign one, but a user ID that isn't a UUID would only cause trouble later.
    if (!vec::isUuid(*userId)) return reject(unauthorized("unauthorized", "the access token is not valid"));

    request->attributes()->insert(userIdAttribute, *userId);
    pass();
}

std::string userIdOf(const drogon::HttpRequestPtr& request) {
    return request->attributes()->get<std::string>(userIdAttribute);
}
