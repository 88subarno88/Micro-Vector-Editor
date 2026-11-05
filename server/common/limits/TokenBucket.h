#ifndef SERVER_TOKEN_BUCKET_H
#define SERVER_TOKEN_BUCKET_H

#include <algorithm>
#include <chrono>

namespace vec {

// Allows bursts of up to `capacity` events, refilled at `perSecond`. Not thread-safe: use one per
// connection, from that connection's thread.
class TokenBucket {
public:
    using Clock = std::chrono::steady_clock;

    TokenBucket(double perSecond, double capacity, Clock::time_point now = Clock::now())
        : perSecond_(perSecond), capacity_(capacity), tokens_(capacity), last_(now) {}

    // Takes a token if there is one; false means the event is over the limit.
    bool tryTake(Clock::time_point now = Clock::now()) {
        const double elapsed = std::chrono::duration<double>(now - last_).count();
        tokens_ = std::min(capacity_, tokens_ + elapsed * perSecond_);
        last_ = now;
        // A hair of slack: refills add up in floating point, and ten 0.1s fall just short of 1.
        if (tokens_ < 1 - 1e-9) return false;
        tokens_ -= 1;
        return true;
    }

private:
    double perSecond_;
    double capacity_;
    double tokens_;
    Clock::time_point last_;
};

}  // namespace vec

#endif  // SERVER_TOKEN_BUCKET_H
