#include "logging/Log.h"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>

using nlohmann::json;

namespace vec {

namespace {

constexpr size_t maxRequestIdLength = 64;

// The current UTC time, e.g. 2026-11-02T10:15:30.123Z.
std::string nowUtc() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t seconds = system_clock::to_time_t(now);
    const auto millis = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

    std::tm utc{};
    gmtime_r(&seconds, &utc);
    char date[32];
    std::strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", &utc);
    char text[40];
    std::snprintf(text, sizeof(text), "%s.%03dZ", date, static_cast<int>(millis));
    return text;
}

}  // namespace

void initLogging() {
    const auto logger = spdlog::stdout_logger_mt("vec");
    logger->set_pattern("%v");
    logger->flush_on(spdlog::level::info);
    spdlog::set_default_logger(logger);
}

std::string formatLogLine(std::string_view level, std::string_view message, std::string_view requestId,
                          const json& fields) {
    json line = {{"time", nowUtc()}, {"level", level}, {"msg", message}};
    if (!requestId.empty()) line["request_id"] = requestId;
    line.update(fields);
    return line.dump(-1, ' ', false, json::error_handler_t::replace);
}

void logInfo(std::string_view message, std::string_view requestId, const json& fields) {
    spdlog::info(formatLogLine("info", message, requestId, fields));
}

void logWarning(std::string_view message, std::string_view requestId, const json& fields) {
    spdlog::warn(formatLogLine("warning", message, requestId, fields));
}

void logError(std::string_view message, std::string_view requestId, const json& fields) {
    spdlog::error(formatLogLine("error", message, requestId, fields));
}

bool isValidRequestId(std::string_view id) {
    if (id.empty() || id.size() > maxRequestIdLength) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
    });
}

}  // namespace vec
