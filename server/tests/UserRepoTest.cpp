#include <regex>
#include <string>

#include "DatabaseTest.h"
#include "db/UserRepo.h"

using namespace vec;

namespace {

class UserRepoTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        email = uniqueEmail();
    }

    std::string email;
};

}  // namespace

TEST_F(UserRepoTest, CreateFillsInGeneratedColumns) {
    UserRepo repo(database);

    const auto user = drogon::sync_wait(repo.create(email, "hash"));

    ASSERT_TRUE(user);
    EXPECT_TRUE(isUuid(user->id)) << user->id;
    EXPECT_EQ(user->email, email);
    EXPECT_EQ(user->passwordHash, "hash");
    EXPECT_FALSE(user->createdAt.empty());
}

TEST_F(UserRepoTest, CreatingATakenEmailIsAnError) {
    UserRepo repo(database);
    ASSERT_TRUE(drogon::sync_wait(repo.create(email, "hash")));

    const auto second = drogon::sync_wait(repo.create(email, "other"));

    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), CreateUserError::EmailTaken);
}

TEST_F(UserRepoTest, FindsByEmailAndById) {
    UserRepo repo(database);
    const auto created = drogon::sync_wait(repo.create(email, "hash"));
    ASSERT_TRUE(created);

    const auto byEmail = drogon::sync_wait(repo.findByEmail(email));
    ASSERT_TRUE(byEmail);
    EXPECT_EQ(byEmail->id, created->id);

    const auto byId = drogon::sync_wait(repo.findById(created->id));
    ASSERT_TRUE(byId);
    EXPECT_EQ(byId->email, email);
    EXPECT_EQ(byId->passwordHash, "hash");
}

TEST_F(UserRepoTest, UnknownUsersAreNotFound) {
    UserRepo repo(database);

    EXPECT_EQ(drogon::sync_wait(repo.findByEmail(email)), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(repo.findById("00000000-0000-0000-0000-000000000000")), std::nullopt);
}

TEST_F(UserRepoTest, AnIdThatIsNotAUuidFindsNothing) {
    UserRepo repo(database);

    EXPECT_EQ(drogon::sync_wait(repo.findById("not-a-uuid")), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(repo.findById("'; DROP TABLE users; --")), std::nullopt);
}

TEST_F(UserRepoTest, EmailsAreParametersNotSql) {
    UserRepo repo(database);
    email = "'; DROP TABLE users; --" + email;

    ASSERT_TRUE(drogon::sync_wait(repo.create(email, "hash")));
    EXPECT_TRUE(drogon::sync_wait(repo.findByEmail(email)));
}

TEST_F(UserRepoTest, TimesAreIso8601InUtc) {
    const auto user = drogon::sync_wait(UserRepo(database).create(email, "hash"));

    ASSERT_TRUE(user);
    EXPECT_TRUE(std::regex_match(user->createdAt, std::regex(R"(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{6}Z)")))
        << user->createdAt;
}
