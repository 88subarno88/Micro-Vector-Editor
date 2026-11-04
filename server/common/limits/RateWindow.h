#ifndef SERVER_RATE_WINDOW_H
#define SERVER_RATE_WINDOW_H

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace vec {

// One fixed window of a rate limit: requests from `client` between two multiples of `length` share a
// counter, and the next window starts fresh.
struct RateWindow {
    std::string key;          // the Redis counter, e.g. "ratelimit:auth:203.0.113.7:29573280"
    int64_t secondsLeft = 0;  // until the window ends, for Retry-After
};

RateWindow rateWindow(std::string_view scope, std::string_view client,
                      std::chrono::system_clock::time_point now, std::chrono::seconds length);

// Increments KEYS[1], giving it a lifetime of ARGV[1] seconds when it's new, and returns the count. One
// script, so that a crash between the two steps can't leave a counter that never expires.
constexpr const char* rateLimitScript =
    "local count = redis.call('INCR', KEYS[1]) "
    "if count == 1 then redis.call('EXPIRE', KEYS[1], ARGV[1]) end "
    "return count";

}  // namespace vec

#endif  // SERVER_RATE_WINDOW_H
