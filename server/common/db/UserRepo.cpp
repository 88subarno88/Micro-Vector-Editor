#include "db/UserRepo.h"

#include <drogon/orm/Result.h>

namespace vec {

namespace {

constexpr const char* columns =
    "id::text, email, password_hash, to_char(created_at AT TIME ZONE 'UTC', "
    "'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"')";

User toUser(const drogon::orm::Row& row) {
    return {row[0].as<std::string>(), row[1].as<std::string>(), row[2].as<std::string>(),
            row[3].as<std::string>()};
}

std::optional<User> firstUser(const drogon::orm::Result& result) {
    if (result.empty()) return std::nullopt;
    return toUser(result[0]);
}

}  // namespace

drogon::Task<tl::expected<User, CreateUserError>> UserRepo::create(const std::string& email,
                                                                   const std::string& passwordHash) {
    // Drogon reports every Postgres error with the same exception type, so a taken email has to be
    // detected here: it inserts nothing and returns no row.
    const auto result = co_await database_->execSqlCoro(
        std::string("INSERT INTO users (email, password_hash) VALUES ($1, $2) "
                    "ON CONFLICT (email) DO NOTHING RETURNING ") +
            columns,
        email, passwordHash);
    if (result.empty()) co_return tl::unexpected(CreateUserError::EmailTaken);
    co_return toUser(result[0]);
}

drogon::Task<std::optional<User>> UserRepo::findByEmail(const std::string& email) {
    co_return firstUser(co_await database_->execSqlCoro(
        std::string("SELECT ") + columns + " FROM users WHERE email = $1", email));
}

drogon::Task<std::optional<User>> UserRepo::findById(const std::string& id) {
    if (!isUuid(id)) co_return std::nullopt;  // Postgres would reject it with an error
    co_return firstUser(co_await database_->execSqlCoro(
        std::string("SELECT ") + columns + " FROM users WHERE id = $1::uuid", id));
}

}  // namespace vec
