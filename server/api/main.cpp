#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <trantor/net/InetAddress.h>

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "ApiResponse.h"
#include "AuthFilter.h"
#include "AuthRoutes.h"
#include "HealthRoutes.h"
#include "RequestLogging.h"
#include "config/Config.h"
#include "logging/Log.h"

namespace {

constexpr size_t databaseConnections = 4;
constexpr size_t redisConnections = 2;
constexpr size_t passwordHashingThreads = 2;

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

    registerRequestLogging();
    registerHealthRoutes(database, redis);

    // Each hash takes about 64 MB and 50 ms, so a few threads bound both memory and CPU.
    const auto hashing = std::make_shared<trantor::EventLoopThreadPool>(passwordHashingThreads, "hashing");
    hashing->start();
    const vec::AccessTokens tokens(config->jwtSecret);
    drogon::app().registerFilter(std::make_shared<AuthFilter>(tokens));
    registerAuthRoutes(database, hashing, tokens);

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
