#ifndef ROUTES_H
#define ROUTES_H

#include <drogon/HttpAppFramework.h>
#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <string>
#include <tl/expected.hpp>
#include <utility>

#include "db/Models.h"

// Registers a route that only signed-in users can reach: AuthFilter is always added, so a protected
// route can't be left open by forgetting it. Use drogon::app().registerHandler only for public routes.
template <typename Handler>
void registerProtected(const std::string& path, Handler&& handler, drogon::HttpMethod method) {
    drogon::app().registerHandler(path, std::forward<Handler>(handler), {method, "AuthFilter"});
}

// The signed-in user's role on a document, if it's at least `minimum`. Otherwise the response to send:
// 404 not_found if they aren't a member (or `documentId` isn't a document), 403 forbidden if their role
// is too low.
drogon::Task<tl::expected<vec::Role, drogon::HttpResponsePtr>> requireRole(
    const drogon::orm::DbClientPtr& database, const std::string& documentId, const std::string& userId,
    vec::Role minimum);

#endif  // ROUTES_H
