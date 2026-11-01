#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <trantor/net/InetAddress.h>

#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "HealthRoutes.h"
#include "RequestLogging.h"
#include "config/Config.h"
#include "logging/Log.h"

namespace {

constexpr size_t databaseConnections = 4;
constexpr size_t redisConnections = 2;

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
    vec::logInfo("server starting", {}, {{"port", config->port}});

    drogon::app().addListener("0.0.0.0", config->port).run();
}
