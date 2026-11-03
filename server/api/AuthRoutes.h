#ifndef AUTH_ROUTES_H
#define AUTH_ROUTES_H

#include <drogon/orm/DbClient.h>
#include <trantor/net/EventLoopThreadPool.h>

#include <memory>

// POST /api/auth/signup {"email", "password"} -> 201 {"id", "email", "createdAt"}
// 400 for malformed JSON, a bad email or a password outside 8..1024 bytes; 409 if the email is taken.
// Password hashing runs on `hashing`, never on a request thread, and its size caps how many run at once.
void registerAuthRoutes(const drogon::orm::DbClientPtr& database,
                        std::shared_ptr<trantor::EventLoopThreadPool> hashing);

#endif  // AUTH_ROUTES_H
