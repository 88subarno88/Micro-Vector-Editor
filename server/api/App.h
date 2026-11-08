#ifndef APP_H
#define APP_H

#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>
#include <trantor/net/InetAddress.h>

#include <optional>
#include <string>

#include "config/Config.h"

// Drogon's Redis client takes an address, not a hostname. nullopt if the host can't be resolved.
std::optional<trantor::InetAddress> resolveRedis(const vec::RedisEndpoint& redis);

// Sets up everything the API serves on drogon::app(): routes, filters, the WebSocket controller, request
// limits and error handling. It doesn't add a listener or start the app. Call once per process. Returns
// this instance's ID, which tells it apart from other instances sharing the same Redis.
std::string configureApp(const vec::Config& config, const drogon::orm::DbClientPtr& database,
                         const drogon::nosql::RedisClientPtr& redis);

#endif  // APP_H
