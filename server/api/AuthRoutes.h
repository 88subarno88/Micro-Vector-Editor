#ifndef AUTH_ROUTES_H
#define AUTH_ROUTES_H

#include <drogon/orm/DbClient.h>
#include <trantor/net/EventLoopThreadPool.h>

#include <memory>

#include "auth/AccessToken.h"

// Every /api/auth route is behind RateLimitFilter.
//
// POST /api/auth/login {"email", "password"} -> 200 {"accessToken", "tokenType", "expiresIn"} and an
// httpOnly refresh_token cookie. 401 invalid_credentials for any wrong email or password, taking the
// same time either way.
//
// POST /api/auth/refresh, with the cookie -> a new access token and refresh cookie, as for login. The old
// refresh token stops working. 401 invalid_refresh_token, and the cookie is cleared, if it's missing,
// expired or revoked; reusing a replaced token also revokes the rest of its login.
//
// POST /api/auth/logout -> 204, revoking the cookie's token if there is one, and clearing the cookie.
//
// GET /api/me, behind AuthFilter -> 200 {"id", "email", "createdAt"} for the signed-in user.
//
// POST /api/auth/signup {"email", "password"} -> 201 {"id", "email", "createdAt"}
// 400 for malformed JSON, a bad email or a password outside 8..1024 bytes; 409 if the email is taken.
// Password hashing runs on `hashing`, never on a request thread, and its size caps how many run at once.
void registerAuthRoutes(const drogon::orm::DbClientPtr& database,
                        std::shared_ptr<trantor::EventLoopThreadPool> hashing, vec::AccessTokens tokens);

#endif  // AUTH_ROUTES_H
