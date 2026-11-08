#include "HttpTest.h"

using nlohmann::json;

namespace {

const json credentials = {{"email", "same@test.invalid"}, {"password", "correct horse"}};

}  // namespace

TEST_F(HttpTest, TheServerAnswers) {
    const Response response = send(drogon::Get, "/healthz");

    EXPECT_EQ(response.status, 200);
}

TEST_F(HttpTest, TheServerUsesTheTestDatabaseAndRedis) {
    const Response response = send(drogon::Get, "/readyz");

    EXPECT_EQ(response.status, 200);
    EXPECT_EQ(response.body["status"], "ready");
}

// The next two sign up the same email; both pass only because the tables are emptied in between.
TEST_F(HttpTest, TablesAreEmptiedBeforeEachTest) {
    EXPECT_EQ(send(drogon::Post, "/api/auth/signup", credentials).status, 201);
}

TEST_F(HttpTest, TablesAreEmptiedBeforeEachTestToo) {
    EXPECT_EQ(send(drogon::Post, "/api/auth/signup", credentials).status, 201);
    EXPECT_EQ(send(drogon::Post, "/api/auth/signup", credentials).status, 409);
}

TEST_F(HttpTest, RateLimitsStartAgainInEachTest) {
    // Auth routes allow 10 requests a minute per IP, and every test comes from 127.0.0.1.
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(send(drogon::Post, "/api/auth/login", credentials).status, 401) << i;
    }
    EXPECT_EQ(send(drogon::Post, "/api/auth/login", credentials).status, 429);
}

TEST_F(HttpTest, ProtectedRoutesNeedAToken) { EXPECT_EQ(send(drogon::Get, "/api/docs").status, 401); }
