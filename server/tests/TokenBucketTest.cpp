#include <gtest/gtest.h>

#include "limits/TokenBucket.h"

using namespace vec;
using namespace std::chrono_literals;

TEST(TokenBucketTest, AllowsABurstUpToCapacity) {
    const auto start = TokenBucket::Clock::now();
    TokenBucket bucket(10, 5, start);

    for (int i = 0; i < 5; ++i) EXPECT_TRUE(bucket.tryTake(start)) << i;
    EXPECT_FALSE(bucket.tryTake(start));
}

TEST(TokenBucketTest, RefillsAtTheRate) {
    const auto start = TokenBucket::Clock::now();
    TokenBucket bucket(10, 5, start);
    for (int i = 0; i < 5; ++i) bucket.tryTake(start);

    EXPECT_FALSE(bucket.tryTake(start + 50ms));  // half a token
    EXPECT_TRUE(bucket.tryTake(start + 100ms));
    EXPECT_FALSE(bucket.tryTake(start + 100ms));
}

TEST(TokenBucketTest, NeverHoldsMoreThanCapacity) {
    const auto start = TokenBucket::Clock::now();
    TokenBucket bucket(10, 5, start);
    const auto later = start + 1h;

    for (int i = 0; i < 5; ++i) EXPECT_TRUE(bucket.tryTake(later)) << i;
    EXPECT_FALSE(bucket.tryTake(later));
}

TEST(TokenBucketTest, SustainsExactlyTheRate) {
    const auto start = TokenBucket::Clock::now();
    TokenBucket bucket(10, 1, start);
    int allowed = 0;
    for (int ms = 0; ms < 10000; ms += 10) allowed += bucket.tryTake(start + std::chrono::milliseconds(ms));

    EXPECT_NEAR(allowed, 100, 1);  // 10 seconds at 10 per second
}
