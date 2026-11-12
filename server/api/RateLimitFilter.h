#ifndef RATE_LIMIT_FILTER_H
#define RATE_LIMIT_FILTER_H

#include <drogon/HttpFilter.h>
#include <drogon/nosql/RedisClient.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "net/ClientAddress.h"

// Allows `limit` requests per client IP in each fixed window, the client being whoever a trusted proxy
// says it is (see clientAddress), counted in Redis so that every server
// instance shares the count; beyond that, 429 rate_limited with Retry-After. If Redis can't be reached,
// requests are let through and the failure is logged: auth stays up, at the cost of the limit. Add it
// to a route as "RateLimitFilter", after registering one instance.
class RateLimitFilter : public drogon::HttpFilter<RateLimitFilter, false> {
public:
    RateLimitFilter(drogon::nosql::RedisClientPtr redis, std::string scope, int limit,
                    std::chrono::seconds window, std::vector<vec::AddressRange> trustedProxies)
        : redis_(std::move(redis)),
          scope_(std::move(scope)),
          limit_(limit),
          window_(window),
          trustedProxies_(std::move(trustedProxies)) {}

    void doFilter(const drogon::HttpRequestPtr& request, drogon::FilterCallback&& reject,
                  drogon::FilterChainCallback&& pass) override;

private:
    drogon::nosql::RedisClientPtr redis_;
    std::string scope_;
    int limit_;
    std::chrono::seconds window_;
    std::vector<vec::AddressRange> trustedProxies_;
};

#endif  // RATE_LIMIT_FILTER_H
