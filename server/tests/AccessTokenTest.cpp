#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "auth/AccessToken.h"

using namespace vec;
using namespace std::chrono_literals;

namespace {

const std::string secret(32, 'k');
const std::string userId = "550e8400-e29b-41d4-a716-446655440000";

std::string base64Url(const std::string& text) {
    static const char* const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    int value = 0, bits = -6;
    for (const unsigned char c : text) {
        value = (value << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(alphabet[(value >> bits) & 0x3F]);
            bits -= 6;
        }
    }
    if (bits > -6) out.push_back(alphabet[((value << 8) >> (bits + 8)) & 0x3F]);
    return out;
}

// The parts of a token: header.payload.signature.
std::string part(const std::string& token, int index) {
    size_t start = 0;
    for (int i = 0; i < index; ++i) start = token.find('.', start) + 1;
    return token.substr(start, token.find('.', start) - start);
}

}  // namespace

TEST(AccessTokenTest, AnIssuedTokenVerifiesToItsUser) {
    const AccessTokens tokens(secret);

    EXPECT_EQ(tokens.verify(tokens.issue(userId)), userId);
}

TEST(AccessTokenTest, ItExpiresAfterFifteenMinutes) {
    const AccessTokens tokens(secret);
    const auto issuedAt = std::chrono::system_clock::now();
    const std::string token = tokens.issue(userId, issuedAt);

    EXPECT_EQ(tokens.verify(token, issuedAt + 14min + 59s), userId);
    const auto expired = tokens.verify(token, issuedAt + 15min + 1s);
    ASSERT_FALSE(expired);
    EXPECT_EQ(expired.error(), TokenError::Expired);
}

TEST(AccessTokenTest, AnotherSecretsTokenIsInvalid) {
    const std::string token = AccessTokens(std::string(32, 'x')).issue(userId);

    const auto result = AccessTokens(secret).verify(token);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), TokenError::Invalid);
}

TEST(AccessTokenTest, ChangingThePayloadBreaksTheSignature) {
    const AccessTokens tokens(secret);
    const std::string token = tokens.issue(userId);
    const std::string forged =
        part(token, 0) + "." +
        base64Url(R"({"iss":"micro-vector-editor","sub":"someone-else","exp":9999999999})") + "." +
        part(token, 2);

    EXPECT_FALSE(tokens.verify(forged));
}

TEST(AccessTokenTest, AnUnsignedNoneTokenIsInvalid) {
    const std::string header = base64Url(R"({"alg":"none","typ":"JWT"})");
    const std::string payload = base64Url(R"({"iss":"micro-vector-editor","sub":"x","exp":9999999999})");

    for (const std::string& token : {header + "." + payload + ".", header + "." + payload}) {
        EXPECT_FALSE(AccessTokens(secret).verify(token)) << token;
    }
}

TEST(AccessTokenTest, ATokenFromAnotherIssuerIsInvalid) {
    const AccessTokens tokens(secret);
    const std::string token = tokens.issue(userId);
    const std::string payload = base64Url(R"({"iss":"someone-else","sub":"x","exp":9999999999})");

    EXPECT_FALSE(tokens.verify(part(token, 0) + "." + payload + "." + part(token, 2)));
}

TEST(AccessTokenTest, GarbageIsInvalidAndNeverThrows) {
    const AccessTokens tokens(secret);
    for (const char* garbage : {"", ".", "..", "a.b.c", "not a token", "eyJhbGciOiJIUzI1NiJ9..",
                                "\xff\xfe.\x01.\x02", "eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ4In0.abc.def"}) {
        const auto result = tokens.verify(garbage);
        ASSERT_FALSE(result) << garbage;
        EXPECT_EQ(result.error(), TokenError::Invalid);
    }
}

TEST(AccessTokenTest, TheTokenDoesNotContainTheSecret) {
    const std::string token = AccessTokens(secret).issue(userId);

    EXPECT_EQ(token.find(secret), std::string::npos);
    EXPECT_EQ(token.find(base64Url(secret)), std::string::npos);
}
