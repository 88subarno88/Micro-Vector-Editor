#include "RequestLogging.h"

#include <drogon/drogon.h>
#include <trantor/utils/Date.h>
#include <trantor/utils/Logger.h>

#include <functional>
#include <string_view>

#include "logging/Log.h"

namespace {

constexpr const char* requestIdHeader = "x-request-id";
constexpr const char* requestIdAttribute = "requestId";

}  // namespace

void registerRequestLogging() {
    // Drogon's own 404 is one cached response shared by every request, so the first request's ID
    // would stick to it. This one is new each time.
    drogon::app().setDefaultHandler(
        [](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& respond) {
            const auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k404NotFound);
            respond(response);
        });

    drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr& request) {
        const std::string& fromClient = request->getHeader(requestIdHeader);
        request->attributes()->insert(
            requestIdAttribute, vec::isValidRequestId(fromClient) ? fromClient : drogon::utils::getUuid());
    });

    drogon::app().registerPreSendingAdvice(
        [](const drogon::HttpRequestPtr& request, const drogon::HttpResponsePtr& response) {
            const std::string id = requestIdOf(request);
            response->addHeader("X-Request-Id", id);

            const double seconds =
                trantor::Date::date().secondsSinceEpoch() - request->creationDate().secondsSinceEpoch();
            // The path only: a query string may hold tokens.
            vec::logInfo("request", id,
                         {{"method", request->getMethodString()},
                          {"path", request->path()},
                          {"status", static_cast<int>(response->statusCode())},
                          {"duration_ms", static_cast<int>(seconds * 1000)}});
        });
}

void routeDrogonLogsToJson() {
    trantor::Logger::setOutputFunction(
        [](const char* text, uint64_t length) {
            std::string_view message(text, length);
            while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
                message.remove_suffix(1);
            }
            if (message.find(" ERROR ") != std::string_view::npos ||
                message.find(" FATAL ") != std::string_view::npos) {
                vec::logError(message);
            } else if (message.find(" WARN ") != std::string_view::npos) {
                vec::logWarning(message);
            } else {
                vec::logInfo(message);
            }
        },
        [] {});
}

std::string requestIdOf(const drogon::HttpRequestPtr& request) {
    return request->attributes()->get<std::string>(requestIdAttribute);
}
