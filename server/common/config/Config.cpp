#include "config/Config.h"

#include <charconv>
#include <cstdlib>
#include <vector>

namespace vec {

namespace {

// Short enough secrets can be brute-forced offline from a single token.
constexpr size_t minJwtSecretLength = 32;

std::optional<uint16_t> parsePort(const std::string& text) {
    uint16_t port = 0;
    const char* const end = text.data() + text.size();
    const auto [parsedTo, error] = std::from_chars(text.data(), end, port);
    if (error != std::errc() || parsedTo != end || port == 0) return std::nullopt;
    return port;
}

}  // namespace

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
    config.redisUrl = require("REDIS_URL");
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
