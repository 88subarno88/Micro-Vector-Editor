#include "Routes.h"

#include "ApiResponse.h"
#include "auth/Access.h"
#include "db/DocRepo.h"

drogon::Task<tl::expected<vec::Role, drogon::HttpResponsePtr>> requireRole(
    const drogon::orm::DbClientPtr& database, const std::string& documentId, const std::string& userId,
    vec::Role minimum) {
    const auto role = co_await vec::MemberRepo(database).roleOf(documentId, userId);
    switch (vec::decideAccess(role, minimum)) {
        case vec::Access::Allowed:
            co_return *role;
        case vec::Access::NotFound:
            co_return tl::unexpected(errorResponse(drogon::k404NotFound, "not_found", "no such document"));
        case vec::Access::Forbidden:
            break;
    }
    co_return tl::unexpected(
        errorResponse(drogon::k403Forbidden, "forbidden",
                      "you need to be " + std::string(vec::toString(minimum)) + " to do this"));
}
