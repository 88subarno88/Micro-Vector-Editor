#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <trantor/net/InetAddress.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "ApiResponse.h"
#include "AuthFilter.h"
#include "AuthRoutes.h"
#include "DocRoutes.h"
#include "HealthRoutes.h"
#include "RateLimitFilter.h"
#include "RequestLogging.h"
#include "collab/DocSocket.h"
#include "config/Config.h"
#include "logging/Log.h"

namespace {

constexpr size_t databaseConnections = 4;
constexpr size_t redisConnections = 2;
constexpr size_t passwordHashingThreads = 2;
constexpr int authRequestsPerMinute = 10;
constexpr double redisTimeoutSeconds = 1.0;
constexpr size_t maxRequestBodyBytes = 1024 * 1024;
constexpr std::chrono::seconds roomUnloadDelay{60};

// Drogon's Redis client takes an address, not a hostname.
std::optional<trantor::InetAddress> resolve(const vec::RedisEndpoint& redis) {
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

}  // namespace

int main() {
    vec::initLogging();
    routeDrogonLogsToJson();

    const auto config = vec::loadConfig();
    if (!config) {
        std::cerr << config.error() << '\n';
        return 1;
    }

    const auto redisAddress = resolve(config->redis);
    if (!redisAddress) {
        std::cerr << "cannot resolve the Redis host " << config->redis.host << '\n';
        return 1;
    }

    const auto database = drogon::orm::DbClient::newPgClient(config->databaseUrl, databaseConnections);
    const auto redis = drogon::nosql::RedisClient::newRedisClient(
        *redisAddress, redisConnections, config->redis.password, config->redis.db, config->redis.username);
    // Drogon would otherwise queue a command until Redis comes back, and the request with it.
    redis->setTimeout(redisTimeoutSeconds);

    registerRequestLogging();
    registerHealthRoutes(database, redis);

    // Each hash takes about 64 MB and 50 ms, so a few threads bound both memory and CPU.
    const auto hashing = std::make_shared<trantor::EventLoopThreadPool>(passwordHashingThreads, "hashing");
    hashing->start();
    const vec::AccessTokens tokens(config->jwtSecret);
    drogon::app().registerFilter(std::make_shared<AuthFilter>(tokens));
    // Enough for real use, but too few to guess passwords or flood signups.
    drogon::app().registerFilter(
        std::make_shared<RateLimitFilter>(redis, "auth", authRequestsPerMinute, std::chrono::minutes(1)));
    registerAuthRoutes(database, hashing, tokens);
    registerDocRoutes(database);
    // One IO thread per core, so that rooms on different documents run in parallel.
    const size_t ioThreads = std::max(1u, std::thread::hardware_concurrency());
    drogon::app().setThreadNum(ioThreads);
    const auto rooms = std::make_shared<vec::RoomRegistry>(
        ioThreads, [](size_t index) { return drogon::app().getIOLoop(index); });
    drogon::app().registerController(std::make_shared<DocSocket>(tokens, database, rooms, roomUnloadDelay));
    // The largest body any route accepts, an SVG upload included. Drogon answers bigger ones with 413.
    drogon::app().setClientMaxBodySize(maxRequestBodyBytes);

    // An exception from a handler, such as a lost database connection: logged, never shown to the client.
    drogon::app().setExceptionHandler([](const std::exception& e, const drogon::HttpRequestPtr& request,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& respond) {
        const std::string id = requestIdOf(request);
        vec::logError("unhandled exception", id, {{"error", e.what()}});
        respond(errorResponse(drogon::k500InternalServerError, "internal", "something went wrong"));
    });
    vec::logInfo("server starting", {}, {{"port", config->port}});

    drogon::app().addListener("0.0.0.0", config->port).run();
}
