#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "DatabaseTest.h"
#include "auth/RefreshToken.h"
#include "db/RefreshTokenRepo.h"

using namespace vec;

TEST(RefreshTokenTest, TokensAre256BitBase64Url) {
    const auto token = refresh::generate();

    EXPECT_TRUE(std::regex_match(token.token, std::regex("[A-Za-z0-9_-]{43}"))) << token.token;
}

TEST(RefreshTokenTest, TokensAreUnique) {
    std::set<std::string> tokens;
    for (int i = 0; i < 1000; ++i) tokens.insert(refresh::generate().token);

    EXPECT_EQ(tokens.size(), 1000u);
}

TEST(RefreshTokenTest, TheHashIsSha256OfTheTokenAndNotTheTokenItself) {
    const auto token = refresh::generate();

    EXPECT_EQ(token.hash, refresh::hashToken(token.token));
    EXPECT_TRUE(std::regex_match(token.hash, std::regex("[0-9a-f]{64}")));
    EXPECT_EQ(token.hash.find(token.token), std::string::npos);
}

TEST(RefreshTokenTest, KnownSha256) {
    EXPECT_EQ(refresh::hashToken("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

namespace {

constexpr std::chrono::hours hour{1};

class RefreshTokenRepoTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        user = newUser();
    }

    // A freshly logged-in token's hash.
    std::string login(std::chrono::seconds lifetime = hour) {
        const std::string hash = refresh::generate().hash;
        drogon::sync_wait(RefreshTokenRepo(database).create(user.id, hash, lifetime));
        return hash;
    }

    // As if `hash` was revoked longer ago than the grace period.
    void ageRevocation(const std::string& hash) {
        database->execSqlSync(
            "UPDATE refresh_tokens SET revoked_at = revoked_at - INTERVAL '1 minute' WHERE token_hash = $1",
            hash);
    }

    tl::expected<std::string, RotateError> rotate(const std::string& oldHash, const std::string& newHash) {
        return drogon::sync_wait(RefreshTokenRepo(database).rotate(oldHash, newHash, hour));
    }

    User user;
};

}  // namespace

TEST_F(RefreshTokenRepoTest, StoresTheHashWithAnExpiry) {
    const std::string hash = login();

    const auto row = database->execSqlSync(
        "SELECT user_id::text, expires_at > now() + INTERVAL '59 minutes', revoked_at IS NULL "
        "FROM refresh_tokens WHERE token_hash = $1",
        hash);
    ASSERT_EQ(row.size(), 1u);
    EXPECT_EQ(row[0][0].as<std::string>(), user.id);
    EXPECT_TRUE(row[0][1].as<bool>());
    EXPECT_TRUE(row[0][2].as<bool>());
}

TEST_F(RefreshTokenRepoTest, RotatingGivesTheUserAndANewWorkingToken) {
    const std::string first = login();
    const std::string second = refresh::generate().hash;

    EXPECT_EQ(rotate(first, second), user.id);
    EXPECT_EQ(rotate(second, refresh::generate().hash), user.id);
}

TEST_F(RefreshTokenRepoTest, ReusingAReplacedTokenRevokesTheWholeFamily) {
    const std::string first = login();
    const std::string second = refresh::generate().hash;
    ASSERT_TRUE(rotate(first, second));
    ageRevocation(first);

    const auto reuse = rotate(first, refresh::generate().hash);

    ASSERT_FALSE(reuse);
    EXPECT_EQ(reuse.error(), RotateError::Reused);
    // The newer token, held by the attacker or the victim, was revoked with the family. Revoked just now,
    // it's inside the grace period, so it's Invalid rather than Reused: refused either way.
    EXPECT_FALSE(rotate(second, refresh::generate().hash));
    ageRevocation(second);
    EXPECT_FALSE(rotate(second, refresh::generate().hash));
}

TEST_F(RefreshTokenRepoTest, ReuseWithinTheGracePeriodIsRefusedButRevokesNothing) {
    const std::string first = login();
    const std::string second = refresh::generate().hash;
    ASSERT_TRUE(rotate(first, second));

    const auto reuse = rotate(first, refresh::generate().hash);

    ASSERT_FALSE(reuse);
    EXPECT_EQ(reuse.error(), RotateError::Invalid);
    EXPECT_EQ(rotate(second, refresh::generate().hash), user.id);
}

TEST_F(RefreshTokenRepoTest, ReuseLeavesOtherLoginsAlone) {
    const std::string phone = login();
    const std::string laptop = login();
    ASSERT_TRUE(rotate(phone, refresh::generate().hash));
    ageRevocation(phone);

    ASSERT_FALSE(rotate(phone, refresh::generate().hash));

    EXPECT_EQ(rotate(laptop, refresh::generate().hash), user.id);
}

TEST_F(RefreshTokenRepoTest, UnknownAndExpiredTokensAreInvalid) {
    const std::string expired = login(std::chrono::seconds(-1));

    for (const std::string& hash : {refresh::generate().hash, expired, std::string()}) {
        const auto result = rotate(hash, refresh::generate().hash);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), RotateError::Invalid);
    }
}

TEST_F(RefreshTokenRepoTest, ALoggedOutTokenCannotBeRefreshed) {
    const std::string token = login();
    RefreshTokenRepo repo(database);

    EXPECT_TRUE(drogon::sync_wait(repo.revoke(token)));
    EXPECT_FALSE(drogon::sync_wait(repo.revoke(token)));
    EXPECT_FALSE(drogon::sync_wait(repo.revoke(refresh::generate().hash)));

    EXPECT_FALSE(rotate(token, refresh::generate().hash));
}

TEST_F(RefreshTokenRepoTest, ConcurrentRefreshesOfOneTokenLetExactlyOneWin) {
    constexpr int attempts = 8;
    const std::string token = login();
    const auto pool = drogon::orm::DbClient::newPgClient(std::getenv("TEST_DATABASE_URL"), attempts);
    new drogon::orm::DbClientPtr(pool);  // never destroyed, for the reason in DatabaseTest

    std::atomic<int> wins = 0;
    std::string winner;
    std::mutex winnerMutex;
    std::vector<std::thread> threads;
    for (int i = 0; i < attempts; ++i) {
        threads.emplace_back([&] {
            const std::string next = refresh::generate().hash;
            if (drogon::sync_wait(RefreshTokenRepo(pool).rotate(token, next, hour))) {
                ++wins;
                const std::lock_guard lock(winnerMutex);
                winner = next;
            }
        });
    }
    for (std::thread& thread : threads) thread.join();

    EXPECT_EQ(wins, 1);
    // Two tabs refreshing at once is normal, not theft: the winner's new token must still work.
    EXPECT_EQ(rotate(winner, refresh::generate().hash), user.id);
}
