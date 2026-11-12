#include "App.h"

#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <trantor/net/InetAddress.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>

#include "ApiResponse.h"
#include "AuthFilter.h"
#include "AuthRoutes.h"
#include "DocRoutes.h"
#include "ExportRoutes.h"
#include "HealthRoutes.h"
#include "RateLimitFilter.h"
#include "RequestLogging.h"
#include "collab/DocSocket.h"
#include "logging/Log.h"

namespace {

constexpr size_t passwordHashingThreads = 2;
constexpr double redisTimeoutSeconds = 1.0;
constexpr size_t maxRequestBodyBytes = 1024 * 1024;
constexpr size_t maxWebSocketMessageBytes = 64 * 1024;
constexpr std::chrono::seconds roomUnloadDelay{60};
constexpr std::chrono::seconds unacknowledgedSendLimit{30};

// Publishes each committed op on the Redis channel doc:{id}, tagged with this instance's ID so that the
// instance can recognise its own messages. A failed publish is only logged: another instance that misses
// the op still catches up from Postgres before its next one.
vec::OpPublisher publishToRedis(drogon::nosql::RedisClientPtr redis, std::string instanceId) {
    return [redis, instanceId](const std::string& documentId, const std::string& message) {
        auto tagged = nlohmann::json::parse(message);
        tagged["instance"] = instanceId;
        redis->execCommandAsync([](const drogon::nosql::RedisResult&) {},
                                [documentId](const std::exception& e) {
                                    vec::logError("publish failed", {},
                                                  {{"document", documentId}, {"error", e.what()}});
                                },
                                "PUBLISH %s %s", ("doc:" + documentId).c_str(), tagged.dump().c_str());
    };
}

}  // namespace

std::optional<trantor::InetAddress> resolveRedis(const vec::RedisEndpoint& redis) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* results = nullptr;
    if (getaddrinfo(redis.host.c_str(), nullptr, &hints, &results) != 0) return std::nullopt;

    const std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> owner(results, freeaddrinfo);
    sockaddr_in address = *reinterpret_cast<const sockaddr_in*>(results->ai_addr);
    address.sin_port = htons(redis.port);
    return trantor::InetAddress(address);
}

std::string configureApp(const vec::Config& config, const drogon::orm::DbClientPtr& database,
                         const drogon::nosql::RedisClientPtr& redis) {
    // Drogon would otherwise queue a command until Redis comes back, and the request with it.
    redis->setTimeout(redisTimeoutSeconds);

    registerRequestLogging();
    registerHealthRoutes(database, redis);

    // Each hash takes about 64 MB and 50 ms, so a few threads bound both memory and CPU.
    const auto hashing = std::make_shared<trantor::EventLoopThreadPool>(passwordHashingThreads, "hashing");
    hashing->start();
    const vec::AccessTokens tokens(config.jwtSecret);
    // Distinguishes this process from other instances of the server sharing the same Redis.
    const std::string instanceId = drogon::utils::getUuid();
    drogon::app().registerFilter(std::make_shared<AuthFilter>(tokens));
    drogon::app().registerFilter(std::make_shared<RateLimitFilter>(
        redis, "auth", config.authRequestsPerMinute, std::chrono::minutes(1), config.trustedProxies));
    registerAuthRoutes(database, hashing, tokens);
    registerDocRoutes(database);
    registerExportRoutes(database, config.exportDir);
    // One IO thread per core, so that rooms on different documents run in parallel.
    const size_t ioThreads = std::max(1u, std::thread::hardware_concurrency());
    drogon::app().setThreadNum(ioThreads);
    const auto rooms = std::make_shared<vec::RoomRegistry>(
        ioThreads, [](size_t index) { return drogon::app().getIOLoop(index); });
    drogon::app().registerController(std::make_shared<DocSocket>(tokens, database, rooms, roomUnloadDelay,
                                                                 publishToRedis(redis, instanceId),
                                                                 redis->newSubscriber(), instanceId, redis));
    // The largest body any route accepts, an SVG upload included. Drogon answers bigger ones with 413.
    drogon::app().setClientMaxBodySize(maxRequestBodyBytes);
    // Drogon makes folders for multipart uploads at startup, by default in the working directory, which in
    // the container isn't writable. No route takes multipart uploads, so they go somewhere harmless.
    drogon::app().setUploadPath((std::filesystem::temp_directory_path() / "vec-uploads").string());
    // The largest WebSocket message a client may send. Ops are small; this leaves room for big addShapes.
    drogon::app().setClientMaxWebSocketMessageSize(maxWebSocketMessageBytes);
    // A client that stops reading lets the server's send buffer for it grow with every broadcast. Drogon
    // doesn't expose that buffer, so the kernel bounds it instead: once sent data has gone unacknowledged
    // for this long, as it does when the client's receive window stays shut, the connection is dropped.
    // The buffer then holds at most this long's worth of broadcasts.
    drogon::app().setAfterAcceptSockOptCallback([](int fd) {
        const unsigned int timeoutMs = std::chrono::milliseconds(unacknowledgedSendLimit).count();
        setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    });

    // An exception from a handler, such as a lost database connection: logged, never shown to the client.
    drogon::app().setExceptionHandler([](const std::exception& e, const drogon::HttpRequestPtr& request,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& respond) {
        const std::string id = requestIdOf(request);
        vec::logError("unhandled exception", id, {{"error", e.what()}});
        respond(errorResponse(drogon::k500InternalServerError, "internal", "something went wrong"));
    });
    return instanceId;
}
