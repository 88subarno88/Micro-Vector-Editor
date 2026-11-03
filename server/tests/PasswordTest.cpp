#include <gtest/gtest.h>

#include <string>

#include "auth/Password.h"

namespace password = vec::password;

TEST(PasswordTest, TheRightPasswordVerifies) {
    const std::string stored = password::hash("correct horse battery staple");

    EXPECT_TRUE(password::verify("correct horse battery staple", stored));
}

TEST(PasswordTest, AWrongPasswordDoesNot) {
    const std::string stored = password::hash("correct horse battery staple");

    for (const char* wrong : {"", "correct horse battery stapl", "correct horse battery staple ",
                              "Correct horse battery staple"}) {
        EXPECT_FALSE(password::verify(wrong, stored)) << wrong;
    }
}

TEST(PasswordTest, HashIsArgon2idAndNeverContainsThePassword) {
    const std::string stored = password::hash("hunter2hunter2");

    EXPECT_EQ(stored.rfind("$argon2id$", 0), 0u) << stored;
    EXPECT_EQ(stored.find("hunter2"), std::string::npos);
}

TEST(PasswordTest, TheSamePasswordHashesDifferentlyEachTime) {
    EXPECT_NE(password::hash("same"), password::hash("same"));
}

TEST(PasswordTest, EmptyLongAndUnusualPasswordsWork) {
    for (const std::string& value :
         {std::string(), std::string(1000, 'x'), std::string("caf\xc3\xa9 \xf0\x9f\x94\x91"),
          std::string("nul\0inside", 10)}) {
        const std::string stored = password::hash(value);
        EXPECT_TRUE(password::verify(value, stored)) << value.size();
    }
}

TEST(PasswordTest, PasswordsAreComparedAsBytesNotStrings) {
    const std::string stored = password::hash(std::string("abc\0def", 7));

    EXPECT_FALSE(password::verify("abc", stored));
}

TEST(PasswordTest, AnythingButAValidHashIsRejected) {
    const std::string stored = password::hash("pw");
    for (const std::string& bad :
         {std::string(), std::string("pw"), std::string("$argon2id$"), stored.substr(0, stored.size() - 1),
          stored + "x", std::string(stored) + '\0' + "x", std::string(500, '$')}) {
        EXPECT_FALSE(password::verify("pw", bad)) << bad;
    }
}
