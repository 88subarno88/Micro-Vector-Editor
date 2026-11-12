#include "HttpTest.h"

#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>
#include <unistd.h>

#include <filesystem>
#include <future>
#include <memory>
#include <thread>

#include "App.h"
#include "config/Config.h"

namespace {

constexpr size_t databaseConnections = 4;
constexpr size_t redisConnections = 2;
constexpr double requestTimeoutSeconds = 10;

TestServer* server = nullptr;

// Starts the server before the first test and stops it after the last, so the process can exit cleanly.
class ServerEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        const auto databaseUrl = vec::readEnv("TEST_DATABASE_URL");
        const auto redisUrl = vec::readEnv("TEST_REDIS_URL");
        if (!databaseUrl || databaseUrl->empty() || !redisUrl || redisUrl->empty()) return;
        const auto redisEndpoint = vec::parseRedisUrl(*redisUrl);
        const auto redisAddress = redisEndpoint ? resolveRedis(*redisEndpoint) : std::nullopt;
        ASSERT_TRUE(redisAddress) << "TEST_REDIS_URL is malformed or its host can't be resolved";

        trantor::Logger::setLogLevel(trantor::Logger::kWarn);
        auto started = std::make_unique<TestServer>();
        started->database = drogon::orm::DbClient::newPgClient(*databaseUrl, databaseConnections);
        started->database->setTimeout(requestTimeoutSeconds);
        started->redis = drogon::nosql::RedisClient::newRedisClient(
            *redisAddress, redisConnections, redisEndpoint->password, redisEndpoint->db,
            redisEndpoint->username);
        started->exportDir =
            (std::filesystem::temp_directory_path() / ("vec-http-test-" + std::to_string(getpid()))).string();
        std::filesystem::create_directories(started->exportDir);

        vec::Config config;
        config.databaseUrl = *databaseUrl;
        config.redis = *redisEndpoint;
        started->jwtSecret = std::string(32, 't');
        config.jwtSecret = started->jwtSecret;
        config.exportDir = started->exportDir;
        config.demoEmail = demoEmail;
        configureApp(config, started->database, started->redis);
        drogon::app().addListener("127.0.0.1", 0);  // any free port

        // Queued now, this runs once the app's loop starts, after the listener is bound.
        std::promise<uint16_t> port;
        drogon::app().getLoop()->queueInLoop(
            [&port] { port.set_value(drogon::app().getListeners().front().toPort()); });
        thread_ = std::thread([] { drogon::app().run(); });
        started->port = port.get_future().get();
        server = started.release();
    }

    void TearDown() override {
        if (!thread_.joinable()) return;
        drogon::app().getLoop()->queueInLoop([] { drogon::app().quit(); });
        thread_.join();
        std::filesystem::remove_all(server->exportDir);
    }

private:
    std::thread thread_;
};

[[maybe_unused]] const auto* const environment = ::testing::AddGlobalTestEnvironment(new ServerEnvironment);

}  // namespace

const TestServer* testServer() { return server; }

void HttpTest::SetUp() {
    server = testServer();
    if (server == nullptr) GTEST_SKIP() << "TEST_DATABASE_URL and TEST_REDIS_URL must both be set";

    // Every table but dbmate's, in one statement so foreign keys between them don't matter.
    server->database->execSqlSync(
        "TRUNCATE users, documents, document_members, operations, snapshots, export_jobs, refresh_tokens");
    server->redis->execCommandSync<int>([](const drogon::nosql::RedisResult&) { return 0; }, "FLUSHDB");
    client_ = drogon::HttpClient::newHttpClient("http://127.0.0.1:" + std::to_string(server->port));
}

drogon::HttpRequestPtr HttpTest::makeRequest(drogon::HttpMethod method, const std::string& path,
                                             const nlohmann::json& body, const std::string& token) {
    auto request = drogon::HttpRequest::newHttpRequest();
    if (!body.is_null()) {
        request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        request->setBody(body.dump());
    }
    request->setMethod(method);
    request->setPath(path);
    if (!token.empty()) request->addHeader("Authorization", "Bearer " + token);
    return request;
}

HttpTest::Response HttpTest::send(const drogon::HttpRequestPtr& request) {
    const auto [result, raw] = client_->sendRequest(request, requestTimeoutSeconds);
    if (result != drogon::ReqResult::Ok || !raw) {
        ADD_FAILURE() << "no response to " << request->path() << ": " << drogon::to_string_view(result);
        return {};
    }
    Response response{static_cast<int>(raw->statusCode()), nullptr, raw};
    if (raw->contentType() == drogon::CT_APPLICATION_JSON) {
        response.body = nlohmann::json::parse(raw->body(), nullptr, false);
    }
    return response;
}

HttpTest::Login HttpTest::logIn(const std::string& email) {
    const Response response =
        send(drogon::Post, "/api/auth/login", {{"email", email}, {"password", password}});
    EXPECT_EQ(response.status, 200) << "logging in " << email;
    if (response.status != 200) return {};
    const Response me = send(drogon::Get, "/api/me", nullptr, response.body.value("accessToken", ""));
    return {me.body.value("id", ""), response.body.value("accessToken", ""),
            response.raw->getCookie("refresh_token").value()};
}

HttpTest::Login HttpTest::signUpAndLogIn(const std::string& email) {
    EXPECT_EQ(send(drogon::Post, "/api/auth/signup", {{"email", email}, {"password", password}}).status, 201)
        << "signing up " << email;
    return logIn(email);
}
