#include <gtest/gtest.h>

#include <thread>

#include "metrics/StageTimer.h"

using namespace vec;
using namespace std::chrono_literals;

TEST(StageTimerTest, EachStageRunsFromTheLastMark) {
    StageTimer timer;
    std::this_thread::sleep_for(20ms);
    timer.mark("first");
    std::this_thread::sleep_for(5ms);
    timer.mark("second");

    const auto times = timer.toJson();

    EXPECT_GE(times.at("first").get<int64_t>(), 20000);
    EXPECT_GE(times.at("second").get<int64_t>(), 5000);
    EXPECT_LT(times.at("second").get<int64_t>(), times.at("first").get<int64_t>());
    // Each figure is rounded down to a microsecond separately, so the parts may sum to slightly less.
    EXPECT_NEAR(times.at("total").get<int64_t>(),
                times.at("first").get<int64_t>() + times.at("second").get<int64_t>(), 2);
}

TEST(StageTimerTest, TimingCanStartEarlier) {
    StageTimer timer(StageTimer::Clock::now() - 1s);
    timer.mark("waited");

    EXPECT_GE(timer.toJson().at("waited").get<int64_t>(), 1000000);
}

TEST(StageTimerTest, NoMarksMeansOnlyATotal) {
    EXPECT_EQ(StageTimer().toJson(), nlohmann::json({{"total", 0}}));
}
