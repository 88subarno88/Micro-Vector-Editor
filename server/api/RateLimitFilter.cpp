#include "RateLimitFilter.h"

#include <drogon/nosql/RedisResult.h>

#include <memory>

#include "ApiResponse.h"
#include "RequestLogging.h"
#include "limits/RateWindow.h"
#include "logging/Log.h"

void RateLimitFilter::doFilter(const drogon::HttpRequestPtr& request, drogon::FilterCallback&& reject,
                               drogon::FilterChainCallback&& pass) {
    const std::string client =
        vec::clientAddress(request->peerAddr().toIp(), request->getHeader("x-real-ip"), trustedProxies_);
    const auto window = vec::rateWindow(scope_, client, std::chrono::system_clock::now(), window_);

    // Redis calls back once, on success or failure, so the two callbacks share them.
    const auto callbacks = std::make_shared<std::pair<drogon::FilterCallback, drogon::FilterChainCallback>>(
        std::move(reject), std::move(pass));
    redis_->execCommandAsync(
        [callbacks, window, limit = limit_](const drogon::nosql::RedisResult& result) {
            if (result.asInteger() <= limit) return callbacks->second();

            const auto response = errorResponse(drogon::k429TooManyRequests, "rate_limited",
                                                "too many requests; try again later");
            response->addHeader("Retry-After", std::to_string(window.secondsLeft));
            callbacks->first(response);
        },
        [callbacks, id = requestIdOf(request)](const std::exception& e) {
            vec::logError("rate limit check failed; letting the request through", id, {{"error", e.what()}});
            callbacks->second();
        },
        "EVAL %s 1 %s %lld", vec::rateLimitScript, window.key.c_str(),
        static_cast<long long>(window_.count()));
}
