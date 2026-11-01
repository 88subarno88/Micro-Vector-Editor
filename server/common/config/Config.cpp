#include "config/Config.h"

#include <charconv>
#include <cstdlib>
#include <string_view>
#include <vector>

namespace vec {

namespace {

// Short enough secrets can be brute-forced offline from a single token.
constexpr size_t minJwtSecretLength = 32;

// The whole of `text` as a number; nullopt if anything else is in it.
template <typename Number>
std::optional<Number> parseNumber(std::string_view text) {
    Number number = 0;
    const char* const end = text.data() + text.size();
    const auto [parsedTo, error] = std::from_chars(text.data(), end, number);
    if (error != std::errc() || parsedTo != end) return std::nullopt;
    return number;
}

std::optional<uint16_t> parsePort(std::string_view text) {
    const auto port = parseNumber<uint16_t>(text);
    if (port == 0) return std::nullopt;
    return port;
}

}  // namespace

std::optional<RedisEndpoint> parseRedisUrl(const std::string& url) {
    constexpr std::string_view scheme = "redis://";
    if (!url.starts_with(scheme)) return std::nullopt;
    std::string_view rest = std::string_view(url).substr(scheme.size());

    RedisEndpoint endpoint;
    // The last '@' ends the credentials, as a password may contain one.
    if (const size_t at = rest.rfind('@'); at != std::string_view::npos) {
        const std::string_view credentials = rest.substr(0, at);
        const size_t colon = credentials.find(':');
        endpoint.username = credentials.substr(0, colon);
        if (colon != std::string_view::npos) endpoint.password = credentials.substr(colon + 1);
        rest.remove_prefix(at + 1);
    }

    if (const size_t slash = rest.find('/'); slash != std::string_view::npos) {
        const auto db = parseNumber<unsigned int>(rest.substr(slash + 1));
        if (!db) return std::nullopt;
        endpoint.db = *db;
        rest = rest.substr(0, slash);
    }

    if (const size_t colon = rest.find(':'); colon != std::string_view::npos) {
        const auto port = parsePort(rest.substr(colon + 1));
        if (!port) return std::nullopt;
        endpoint.port = *port;
        rest = rest.substr(0, colon);
    }

    if (rest.empty()) return std::nullopt;
    endpoint.host = rest;
    return endpoint;
}

std::optional<std::string> readEnv(const std::string& name) {
    const char* const value = std::getenv(name.c_str());
    if (value == nullptr) return std::nullopt;
    return std::string(value);
}

tl::expected<Config, std::string> loadConfig(const EnvLookup& lookup) {
    std::vector<std::string> problems;
    const auto require = [&](const char* name) {
        std::string value = lookup(name).value_or("");
        if (value.empty()) problems.push_back(std::string(name) + " is not set");
        return value;
    };

    Config config;
    config.databaseUrl = require("DATABASE_URL");
    if (!config.databaseUrl.empty() && !config.databaseUrl.starts_with("postgres://") &&
        !config.databaseUrl.starts_with("postgresql://")) {
        problems.push_back("DATABASE_URL must start with postgres:// or postgresql://");
    }

    const std::string redisUrl = require("REDIS_URL");
    if (!redisUrl.empty()) {
        if (const auto redis = parseRedisUrl(redisUrl)) {
            config.redis = *redis;
        } else {
            problems.push_back("REDIS_URL must look like redis://[user:password@]host[:port][/db]");
        }
    }

    config.jwtSecret = require("JWT_SECRET");
    if (!config.jwtSecret.empty() && config.jwtSecret.size() < minJwtSecretLength) {
        problems.push_back("JWT_SECRET must be at least " + std::to_string(minJwtSecretLength) +
                           " characters");
    }

    const std::string port = require("PORT");
    if (!port.empty()) {
        const auto parsed = parsePort(port);
        if (parsed)
            config.port = *parsed;
        else
            problems.push_back("PORT must be a number from 1 to 65535");
    }

    if (problems.empty()) return config;

    std::string message = "invalid configuration:";
    for (const std::string& problem : problems) message += "\n  " + problem;
    return tl::unexpected(message);
}

}  // namespace vec
