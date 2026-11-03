#ifndef AUTH_FILTER_H
#define AUTH_FILTER_H

#include <drogon/HttpFilter.h>

#include <string>

#include "auth/AccessToken.h"

// Lets a request through only with a valid "Authorization: Bearer <access token>" header, and records
// the user for the handler (see userIdOf). Otherwise answers 401: token_expired, so that a client knows
// to refresh, or unauthorized. Add it to a route as "AuthFilter", after registering one instance.
class AuthFilter : public drogon::HttpFilter<AuthFilter, false> {
public:
    explicit AuthFilter(vec::AccessTokens tokens) : tokens_(std::move(tokens)) {}

    void doFilter(const drogon::HttpRequestPtr& request, drogon::FilterCallback&& reject,
                  drogon::FilterChainCallback&& pass) override;

private:
    vec::AccessTokens tokens_;
};

// The signed-in user's ID, on a route behind AuthFilter.
std::string userIdOf(const drogon::HttpRequestPtr& request);

#endif  // AUTH_FILTER_H
