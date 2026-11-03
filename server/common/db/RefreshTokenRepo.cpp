#include "db/RefreshTokenRepo.h"

#include <cstdint>

#include "db/OpRepo.h"

namespace vec {

drogon::Task<> RefreshTokenRepo::create(const std::string& userId, const std::string& tokenHash,
                                        std::chrono::seconds lifetime) {
    co_await database_->execSqlCoro(
        "INSERT INTO refresh_tokens (user_id, token_hash, expires_at, family_id) "
        "VALUES ($1::uuid, $2, now() + $3::bigint * INTERVAL '1 second', gen_random_uuid())",
        userId, tokenHash, static_cast<int64_t>(lifetime.count()));
}

drogon::Task<tl::expected<std::string, RotateError>> RefreshTokenRepo::rotate(const std::string& oldHash,
                                                                              const std::string& newHash,
                                                                              std::chrono::seconds lifetime) {
    auto transaction = co_await database_->newTransactionCoro();

    // Only one request can revoke a live token, so only one wins a race.
    const auto claimed = co_await transaction->execSqlCoro(
        "UPDATE refresh_tokens SET revoked_at = now() "
        "WHERE token_hash = $1 AND revoked_at IS NULL AND expires_at > now() "
        "RETURNING user_id::text, family_id::text",
        oldHash);

    if (claimed.empty()) {
        // A token revoked moments ago was most likely raced by another tab of the same browser, which
        // already has the new token: just refuse. Later reuse means a copy is being replayed.
        const auto reused = co_await transaction->execSqlCoro(
            "UPDATE refresh_tokens SET revoked_at = coalesce(revoked_at, now()) "
            "WHERE family_id = (SELECT family_id FROM refresh_tokens WHERE token_hash = $1 "
            "                   AND revoked_at < now() - $2::bigint * INTERVAL '1 second')",
            oldHash, static_cast<int64_t>(reuseGracePeriod.count()));
        co_await commit(std::move(transaction));
        co_return tl::unexpected(reused.affectedRows() > 0 ? RotateError::Reused : RotateError::Invalid);
    }

    const std::string userId = claimed[0][0].as<std::string>();
    co_await transaction->execSqlCoro(
        "INSERT INTO refresh_tokens (user_id, token_hash, expires_at, family_id) "
        "VALUES ($1::uuid, $2, now() + $3::bigint * INTERVAL '1 second', $4::uuid)",
        userId, newHash, static_cast<int64_t>(lifetime.count()), claimed[0][1].as<std::string>());
    co_await commit(std::move(transaction));
    co_return userId;
}

drogon::Task<bool> RefreshTokenRepo::revoke(const std::string& tokenHash) {
    const auto result = co_await database_->execSqlCoro(
        "UPDATE refresh_tokens SET revoked_at = now() WHERE token_hash = $1 AND revoked_at IS NULL",
        tokenHash);
    co_return result.affectedRows() > 0;
}

}  // namespace vec
