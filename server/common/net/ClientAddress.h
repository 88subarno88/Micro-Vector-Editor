#ifndef SERVER_CLIENT_ADDRESS_H
#define SERVER_CLIENT_ADDRESS_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vec {

// A block of IPv4 addresses, such as 10.0.0.0/8.
struct AddressRange {
    uint32_t network = 0;
    uint32_t mask = 0;
};

// "10.0.0.0/8", or one address, "10.1.2.3". nullopt if it's anything else, IPv6 included.
std::optional<AddressRange> parseAddressRange(std::string_view text);

// Ranges separated by commas, spaces allowed around them; an empty list for empty text. nullopt if any of
// them doesn't parse.
std::optional<std::vector<AddressRange>> parseAddressRanges(std::string_view text);

// Who sent a request. Behind a proxy, every request comes from the proxy, which says who it's passing on
// in X-Real-IP; but anyone can send that header. So it's believed only from `trusted` proxies, which are
// IPv4, and only if it holds an address, IPv4 or IPv6. Otherwise the client is the peer itself.
std::string clientAddress(const std::string& peer, const std::string& realIp,
                          const std::vector<AddressRange>& trusted);

}  // namespace vec

#endif  // SERVER_CLIENT_ADDRESS_H
