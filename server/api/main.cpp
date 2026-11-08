#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>

#include <iostream>

#include "App.h"
#include "RequestLogging.h"
#include "config/Config.h"
#include "logging/Log.h"

namespace {

constexpr size_t databaseConnections = 4;
constexpr size_t redisConnections = 2;

}  // namespace

int main() {
    vec::initLogging();
    routeDrogonLogsToJson();

    const auto config = vec::loadConfig();
    if (!config) {
        std::cerr << config.error() << '\n';
        return 1;
    }

    const auto redisAddress = resolveRedis(config->redis);
    if (!redisAddress) {
        std::cerr << "cannot resolve the Redis host " << config->redis.host << '\n';
        return 1;
    }

    const auto database = drogon::orm::DbClient::newPgClient(config->databaseUrl, databaseConnections);
    const auto redis = drogon::nosql::RedisClient::newRedisClient(
        *redisAddress, redisConnections, config->redis.password, config->redis.db, config->redis.username);
    const std::string instanceId = configureApp(*config, database, redis);
    vec::logInfo("server starting", {}, {{"port", config->port}, {"instance", instanceId}});

    drogon::app().addListener("0.0.0.0", config->port).run();
}
