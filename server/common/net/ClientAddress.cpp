#include "net/ClientAddress.h"

#include <arpa/inet.h>

#include <charconv>

namespace vec {

namespace {

// An IPv4 address in host byte order; nullopt for anything else.
std::optional<uint32_t> parseIpv4(std::string_view text) {
    const std::string address(text);
    in_addr parsed{};
    if (inet_pton(AF_INET, address.c_str(), &parsed) != 1) return std::nullopt;
    return ntohl(parsed.s_addr);
}

std::string_view trimmed(std::string_view text) {
    const auto first = text.find_first_not_of(' ');
    if (first == std::string_view::npos) return {};
    return text.substr(first, text.find_last_not_of(' ') - first + 1);
}

}  // namespace

std::optional<AddressRange> parseAddressRange(std::string_view text) {
    const auto slash = text.find('/');
    const auto address = parseIpv4(text.substr(0, slash));
    if (!address) return std::nullopt;

    int bits = 32;
    if (slash != std::string_view::npos) {
        const std::string_view prefix = text.substr(slash + 1);
        const auto [end, error] = std::from_chars(prefix.data(), prefix.data() + prefix.size(), bits);
        if (error != std::errc() || end != prefix.data() + prefix.size() || prefix.empty() || bits < 0 ||
            bits > 32) {
            return std::nullopt;
        }
    }
    const uint32_t mask = bits == 0 ? 0 : ~uint32_t{0} << (32 - bits);
    return AddressRange{*address & mask, mask};
}

std::optional<std::vector<AddressRange>> parseAddressRanges(std::string_view text) {
    std::vector<AddressRange> ranges;
    if (trimmed(text).empty()) return ranges;
    while (true) {
        const auto comma = text.find(',');
        const auto range = parseAddressRange(trimmed(text.substr(0, comma)));
        if (!range) return std::nullopt;
        ranges.push_back(*range);
        if (comma == std::string_view::npos) return ranges;
        text.remove_prefix(comma + 1);
    }
}

std::string clientAddress(const std::string& peer, const std::string& realIp,
                          const std::vector<AddressRange>& trusted) {
    const auto peerAddress = parseIpv4(peer);
    if (!peerAddress || !parseIpv4(realIp)) return peer;
    for (const AddressRange& range : trusted) {
        if ((*peerAddress & range.mask) == range.network) return realIp;
    }
    return peer;
}

}  // namespace vec
