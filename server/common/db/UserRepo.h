#ifndef SERVER_USER_REPO_H
#define SERVER_USER_REPO_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <optional>
#include <string>
#include <tl/expected.hpp>

#include "db/Models.h"

namespace vec {

enum class CreateUserError { EmailTaken };

// Reads and writes the users table. Unexpected database failures throw DrogonDbException.
class UserRepo {
public:
    explicit UserRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // Emails are compared exactly, so callers should normalise them first.
    drogon::Task<tl::expected<User, CreateUserError>> create(const std::string& email,
                                                             const std::string& passwordHash);
    drogon::Task<std::optional<User>> findByEmail(const std::string& email);

    // An `id` that isn't a UUID finds nothing.
    drogon::Task<std::optional<User>> findById(const std::string& id);

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_USER_REPO_H
