#include <gtest/gtest.h>

#include <map>
#include <string>

#include "config/Config.h"

using namespace vec;

namespace {

const std::string goodSecret(32, 's');

std::map<std::string, std::string> validEnv() {
    return {{"PORT", "8080"},
            {"DATABASE_URL", "postgres://db"},
            {"REDIS_URL", "redis://cache"},
            {"JWT_SECRET", goodSecret}};
}

auto load(const std::map<std::string, std::string>& env) {
    return loadConfig([&](const std::string& name) -> std::optional<std::string> {
        const auto it = env.find(name);
        if (it == env.end()) return std::nullopt;
        return it->second;
    });
}

}  // namespace

TEST(ConfigTest, LoadsEverySetting) {
    const auto config = load(validEnv());

    ASSERT_TRUE(config) << config.error();
    EXPECT_EQ(config->port, 8080);
    EXPECT_EQ(config->databaseUrl, "postgres://db");
    EXPECT_EQ(config->redisUrl, "redis://cache");
    EXPECT_EQ(config->jwtSecret, goodSecret);
}

TEST(ConfigTest, EachMissingOrEmptySettingIsNamed) {
    for (const char* name : {"PORT", "DATABASE_URL", "REDIS_URL", "JWT_SECRET"}) {
        auto missing = validEnv();
        missing.erase(name);
        const auto withoutIt = load(missing);
        ASSERT_FALSE(withoutIt) << name;
        EXPECT_NE(withoutIt.error().find(name), std::string::npos);

        auto empty = validEnv();
        empty[name] = "";
        const auto blank = load(empty);
        ASSERT_FALSE(blank) << name;
        EXPECT_NE(blank.error().find(name), std::string::npos);
    }
}

TEST(ConfigTest, ReportsEveryProblemAtOnce) {
    const auto config = load({});

    ASSERT_FALSE(config);
    for (const char* name : {"PORT", "DATABASE_URL", "REDIS_URL", "JWT_SECRET"}) {
        EXPECT_NE(config.error().find(name), std::string::npos) << name;
    }
}

TEST(ConfigTest, RejectsABadPort) {
    for (const char* port : {"0", "65536", "-1", "abc", "80a", " 80", "80.5"}) {
        auto env = validEnv();
        env["PORT"] = port;
        const auto config = load(env);
        ASSERT_FALSE(config) << port;
        EXPECT_NE(config.error().find("PORT"), std::string::npos) << port;
    }
}

TEST(ConfigTest, AcceptsTheEndsOfThePortRange) {
    for (const auto& [text, port] : {std::pair{"1", 1}, std::pair{"65535", 65535}}) {
        auto env = validEnv();
        env["PORT"] = text;
        const auto config = load(env);
        ASSERT_TRUE(config) << text;
        EXPECT_EQ(config->port, port);
    }
}

TEST(ConfigTest, RejectsAShortSecretWithoutPrintingIt) {
    auto env = validEnv();
    env["JWT_SECRET"] = "hunter2";

    const auto config = load(env);

    ASSERT_FALSE(config);
    EXPECT_NE(config.error().find("JWT_SECRET"), std::string::npos);
    EXPECT_EQ(config.error().find("hunter2"), std::string::npos);
}

TEST(ConfigTest, ReadEnvSeesTheProcessEnvironment) {
    EXPECT_EQ(readEnv("VEC_CONFIG_TEST_NOT_SET"), std::nullopt);
    ASSERT_EQ(setenv("VEC_CONFIG_TEST_SET", "value", 1), 0);
    EXPECT_EQ(readEnv("VEC_CONFIG_TEST_SET"), "value");
}
