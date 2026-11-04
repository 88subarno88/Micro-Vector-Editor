#ifndef SERVER_STAGE_TIMER_H
#define SERVER_STAGE_TIMER_H

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace vec {

// Times consecutive stages of one piece of work: each mark() ends a stage that began at the previous mark,
// or at the start.
class StageTimer {
public:
    using Clock = std::chrono::steady_clock;

    explicit StageTimer(Clock::time_point start = Clock::now()) : start_(start), last_(start) {}

    void mark(std::string stage) {
        const auto now = Clock::now();
        stages_.emplace_back(std::move(stage), now - last_);
        last_ = now;
    }

    // {"<stage>": microseconds, ..., "total": microseconds}.
    nlohmann::json toJson() const {
        nlohmann::json out = nlohmann::json::object();
        for (const auto& [stage, duration] : stages_) out[stage] = micros(duration);
        out["total"] = micros(last_ - start_);
        return out;
    }

private:
    static int64_t micros(Clock::duration duration) {
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

    Clock::time_point start_;
    Clock::time_point last_;
    std::vector<std::pair<std::string, Clock::duration>> stages_;
};

}  // namespace vec

#endif  // SERVER_STAGE_TIMER_H
