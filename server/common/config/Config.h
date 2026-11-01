#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <tl/expected.hpp>

namespace vec {

// Where Redis is, from a URL such as redis://user:password@host:6379/0.
struct RedisEndpoint {
    std::string host;
    uint16_t port = 6379;
    std::string username;
    std::string password;
    unsigned int db = 0;
};

// Parses a redis:// URL; nullopt if it's malformed. Percent-encoding isn't decoded.
std::optional<RedisEndpoint> parseRedisUrl(const std::string& url);

// Everything the server needs from its environment.
struct Config {
    uint16_t port = 0;
    std::string databaseUrl;
    RedisEndpoint redis;
    std::string jwtSecret;
};

// Looks up one setting by name; nullopt if it isn't set.
using EnvLookup = std::function<std::optional<std::string>(const std::string&)>;

// Reads the process environment.
std::optional<std::string> readEnv(const std::string& name);

// Builds a Config from PORT, DATABASE_URL, REDIS_URL and JWT_SECRET. An empty value counts as
// missing, and the two URLs must be well formed. The error lists every problem at once and never contains a
// setting's value.
tl::expected<Config, std::string> loadConfig(const EnvLookup& lookup = readEnv);

}  // namespace vec

#endif  // SERVER_CONFIG_H
