#ifndef SERVER_HTTP_TEST_H
#define SERVER_HTTP_TEST_H

#include <drogon/HttpClient.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

// The real API, started once per test process on 127.0.0.1 and a free port, with its own database and
// Redis clients for tests to look behind it.
struct TestServer {
    uint16_t port = 0;
    drogon::orm::DbClientPtr database;
    drogon::nosql::RedisClientPtr redis;
    std::string exportDir;
    std::string jwtSecret;  // for tests that make their own tokens
};

// The test server's demo user, as DEMO_EMAIL would name it. Tests create it when they need it.
inline constexpr const char* demoEmail = "demo@example.com";

// The running server; nullptr if TEST_DATABASE_URL or TEST_REDIS_URL isn't set.
const TestServer* testServer();

// Base for tests that talk HTTP to the API. Before each test, every table in TEST_DATABASE_URL and the
// whole Redis database in TEST_REDIS_URL are emptied, so point both at databases that hold nothing else.
// Skipped if either isn't set.
class HttpTest : public ::testing::Test {
protected:
    void SetUp() override;

    struct Response {
        int status = 0;
        nlohmann::json body;  // null unless the response was JSON
        drogon::HttpResponsePtr raw;
    };

    // A request whose `body`, unless null, is JSON, and with `token`, if not empty, as a bearer token.
    static drogon::HttpRequestPtr makeRequest(drogon::HttpMethod method, const std::string& path,
                                              const nlohmann::json& body = nullptr,
                                              const std::string& token = "");

    // Sends a request and waits for the answer. Fails the test if no response comes back.
    Response send(const drogon::HttpRequestPtr& request);
    Response send(drogon::HttpMethod method, const std::string& path, const nlohmann::json& body = nullptr,
                  const std::string& token = "") {
        return send(makeRequest(method, path, body, token));
    }

    static constexpr const char* password = "correct horse";

    struct Login {
        std::string userId;
        std::string accessToken;
        std::string refreshToken;  // from the refresh_token cookie
    };

    // Logs in with `password`; fails the test unless that works. One request toward the auth rate limit.
    Login logIn(const std::string& email);
    // Signs up, then logs in: two requests toward the auth rate limit of 10 a minute.
    Login signUpAndLogIn(const std::string& email);

    const TestServer* server = nullptr;

private:
    drogon::HttpClientPtr client_;
};

#endif  // SERVER_HTTP_TEST_H
