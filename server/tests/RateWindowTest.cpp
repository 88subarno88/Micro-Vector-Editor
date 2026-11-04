#include <gtest/gtest.h>

#include <chrono>

#include "limits/RateWindow.h"

using namespace vec;
using namespace std::chrono_literals;

namespace {

std::chrono::system_clock::time_point at(int64_t seconds) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
}

}  // namespace

TEST(RateWindowTest, KeyNamesTheScopeClientAndWindow) {
    EXPECT_EQ(rateWindow("auth", "203.0.113.7", at(120), 60s).key, "ratelimit:auth:203.0.113.7:2");
}

TEST(RateWindowTest, RequestsInOneWindowShareAKey) {
    EXPECT_EQ(rateWindow("auth", "ip", at(120), 60s).key, rateWindow("auth", "ip", at(179), 60s).key);
}

TEST(RateWindowTest, TheNextWindowGetsANewKey) {
    EXPECT_NE(rateWindow("auth", "ip", at(179), 60s).key, rateWindow("auth", "ip", at(180), 60s).key);
}

TEST(RateWindowTest, ClientsAndScopesAreCountedApart) {
    EXPECT_NE(rateWindow("auth", "1.1.1.1", at(0), 60s).key, rateWindow("auth", "2.2.2.2", at(0), 60s).key);
    EXPECT_NE(rateWindow("auth", "ip", at(0), 60s).key, rateWindow("other", "ip", at(0), 60s).key);
}

TEST(RateWindowTest, SecondsLeftCountsDownToTheNextWindow) {
    EXPECT_EQ(rateWindow("auth", "ip", at(120), 60s).secondsLeft, 60);
    EXPECT_EQ(rateWindow("auth", "ip", at(150), 60s).secondsLeft, 30);
    EXPECT_EQ(rateWindow("auth", "ip", at(179), 60s).secondsLeft, 1);
}
