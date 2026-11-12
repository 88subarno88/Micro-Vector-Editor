#include <gtest/gtest.h>

#include "net/ClientAddress.h"

using namespace vec;

namespace {

std::vector<AddressRange> ranges(std::string_view text) { return parseAddressRanges(text).value(); }

}  // namespace

TEST(ClientAddressTest, ParsesRangesAndSingleAddresses) {
    const auto range = parseAddressRange("10.1.2.3/8");
    ASSERT_TRUE(range);
    EXPECT_EQ(range->network, 0x0A000000u);  // the host part dropped
    EXPECT_EQ(range->mask, 0xFF000000u);

    EXPECT_EQ(parseAddressRange("127.0.0.1")->mask, 0xFFFFFFFFu);
    EXPECT_EQ(parseAddressRange("0.0.0.0/0")->mask, 0u);  // everyone
}

TEST(ClientAddressTest, RejectsWhatIsNotARange) {
    for (const char* bad : {"", "10.0.0", "10.0.0.256", "10.0.0.0/33", "10.0.0.0/-1", "10.0.0.0/",
                            "10.0.0.0/8x", "::1", "example.com"}) {
        EXPECT_FALSE(parseAddressRange(bad)) << bad;
    }
}

TEST(ClientAddressTest, ParsesAListAndRejectsItIfAnyPartIsBad) {
    EXPECT_EQ(ranges(" 10.0.0.0/8, 172.16.0.0/12 ,192.168.0.0/16").size(), 3u);
    EXPECT_TRUE(ranges("").empty());
    EXPECT_TRUE(ranges("  ").empty());
    EXPECT_FALSE(parseAddressRanges("10.0.0.0/8,nonsense"));
    EXPECT_FALSE(parseAddressRanges("10.0.0.0/8,"));
}

TEST(ClientAddressTest, BelievesXRealIpOnlyFromATrustedProxy) {
    const auto proxies = ranges("172.16.0.0/12");

    EXPECT_EQ(clientAddress("172.18.0.5", "203.0.113.7", proxies), "203.0.113.7");     // from the proxy
    EXPECT_EQ(clientAddress("198.51.100.9", "203.0.113.7", proxies), "198.51.100.9");  // forged by a client
    EXPECT_EQ(clientAddress("172.18.0.5", "203.0.113.7", {}), "172.18.0.5");           // nobody trusted
}

// Through Cloudflare, a visitor's address is often IPv6, while the proxy in front is on Docker's IPv4.
TEST(ClientAddressTest, TakesAnIpv6ClientFromATrustedProxy) {
    EXPECT_EQ(clientAddress("172.18.0.5", "2001:db8::7", ranges("172.16.0.0/12")), "2001:db8::7");
    EXPECT_EQ(clientAddress("198.51.100.9", "2001:db8::7", ranges("172.16.0.0/12")), "198.51.100.9");
}

TEST(ClientAddressTest, IgnoresAMissingOrMalformedHeader) {
    const auto proxies = ranges("172.16.0.0/12");
    EXPECT_EQ(clientAddress("172.18.0.5", "", proxies), "172.18.0.5");
    EXPECT_EQ(clientAddress("172.18.0.5", "not an address", proxies), "172.18.0.5");
    EXPECT_EQ(clientAddress("172.18.0.5", "203.0.113.7, 10.0.0.1", proxies), "172.18.0.5");  // a list
}
