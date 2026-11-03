#include <gtest/gtest.h>

#include <string>

#include "auth/Credentials.h"

using namespace vec;

TEST(CredentialsTest, EmailsAreTrimmedAndLowercased) {
    EXPECT_EQ(normalizeEmail("  Ann.Lee@Example.COM\t"), "ann.lee@example.com");
    EXPECT_EQ(normalizeEmail("a+tag@sub.example.org"), "a+tag@sub.example.org");
}

TEST(CredentialsTest, NonAsciiIsKeptAsItIs) {
    EXPECT_EQ(normalizeEmail("J\xc3\x96RG@example.com"), "j\xc3\x96rg@example.com");
}

TEST(CredentialsTest, MalformedEmailsAreRejected) {
    for (const char* email : {"", "   ", "ann", "@example.com", "ann@", "ann@example", "ann@.com",
                              "ann@example.", "ann@@example.com", "a@b@example.com", "ann lee@example.com",
                              "ann@exa mple.com", "ann\n@example.com", "ann\x7f@example.com"}) {
        EXPECT_EQ(normalizeEmail(email), std::nullopt) << email;
    }
}

TEST(CredentialsTest, EmailLengthIsCapped) {
    const std::string domain = "@example.com";
    EXPECT_TRUE(normalizeEmail(std::string(254 - domain.size(), 'a') + domain));
    EXPECT_FALSE(normalizeEmail(std::string(255 - domain.size(), 'a') + domain));
}

TEST(CredentialsTest, PasswordLengthLimits) {
    EXPECT_FALSE(isAcceptablePassword(""));
    EXPECT_FALSE(isAcceptablePassword(std::string(7, 'x')));
    EXPECT_TRUE(isAcceptablePassword(std::string(8, 'x')));
    EXPECT_TRUE(isAcceptablePassword(std::string(1024, 'x')));
    EXPECT_FALSE(isAcceptablePassword(std::string(1025, 'x')));
}
