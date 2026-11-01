#ifndef SERVER_DATABASE_TEST_H
#define SERVER_DATABASE_TEST_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/Utilities.h>
#include <drogon/utils/coroutine.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "db/UserRepo.h"

// Base for tests that need a migrated database: set TEST_DATABASE_URL, or they are skipped. Users from
// newUser() are deleted afterwards, and their documents with them.
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* const url = std::getenv("TEST_DATABASE_URL");
        if (url == nullptr) GTEST_SKIP() << "TEST_DATABASE_URL is not set";
        database = sharedClient(url);
        marker = drogon::utils::getUuid();
    }

    void TearDown() override {
        if (!database) return;
        const std::string pattern = "%" + marker + "%";
        database->execSqlSync(
            "DELETE FROM documents WHERE owner_id IN (SELECT id FROM users WHERE email LIKE $1)", pattern);
        database->execSqlSync("DELETE FROM users WHERE email LIKE $1", pattern);
    }

    // An email unique to this test, which TearDown() cleans up.
    std::string uniqueEmail(const std::string& prefix = "user") const {
        return prefix + "-" + marker + "@test.invalid";
    }

    vec::User newUser(const std::string& prefix = "user") {
        return *drogon::sync_wait(vec::UserRepo(database).create(uniqueEmail(prefix), "hash"));
    }

    drogon::orm::DbClientPtr database;
    std::string marker;

private:
    // One client that's never destroyed, as in the server. Destroying one per test crashes now and then:
    // if the last reference goes on the client's own thread, that thread tries to join itself.
    static drogon::orm::DbClientPtr sharedClient(const char* url) {
        static const auto* const client =
            new drogon::orm::DbClientPtr(drogon::orm::DbClient::newPgClient(url, 1));
        return *client;
    }
};

#endif  // SERVER_DATABASE_TEST_H
