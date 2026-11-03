#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include <drogon/HttpResponse.h>

#include <nlohmann/json.hpp>
#include <string>

// A JSON response from an nlohmann value.
drogon::HttpResponsePtr jsonResponse(drogon::HttpStatusCode status, const nlohmann::json& body);

// Every API error has this shape: {"error": {"code": "email_taken", "message": "..."}}. `code` is for
// programs and stays stable; `message` is for people.
drogon::HttpResponsePtr errorResponse(drogon::HttpStatusCode status, const std::string& code,
                                      const std::string& message);

#endif  // API_RESPONSE_H
