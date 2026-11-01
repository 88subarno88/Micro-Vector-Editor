#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace vec {

// Sends logs to stdout, one JSON object per line. Call once at startup.
void initLogging();

// One log line: {"time":..., "level":..., "msg":..., "request_id":..., <fields>}. request_id is left
// out when empty. Bytes that aren't valid UTF-8 are replaced, so any input gives valid JSON.
std::string formatLogLine(std::string_view level, std::string_view message, std::string_view requestId = {},
                          const nlohmann::json& fields = nlohmann::json::object());

void logInfo(std::string_view message, std::string_view requestId = {},
             const nlohmann::json& fields = nlohmann::json::object());
void logWarning(std::string_view message, std::string_view requestId = {},
                const nlohmann::json& fields = nlohmann::json::object());
void logError(std::string_view message, std::string_view requestId = {},
              const nlohmann::json& fields = nlohmann::json::object());

// Whether a client's X-Request-Id may be used as is: 1 to 64 letters, digits, '.', '_' or '-'.
// Anything else could inject text into logs or response headers.
bool isValidRequestId(std::string_view id);

}  // namespace vec

#endif  // SERVER_LOG_H
