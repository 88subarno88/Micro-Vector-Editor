#ifndef REQUEST_LOGGING_H
#define REQUEST_LOGGING_H

#include <drogon/HttpRequest.h>

#include <string>

// Gives every request an ID (its X-Request-Id if that's safe, or a new one), returns it in the
// response's X-Request-Id, and logs each request when its response goes out.
void registerRequestLogging();

// Sends Drogon's own log output through the JSON logger. These lines have no request ID.
void routeDrogonLogsToJson();

// The ID of `request`, for log lines written while handling it.
std::string requestIdOf(const drogon::HttpRequestPtr& request);

#endif  // REQUEST_LOGGING_H
