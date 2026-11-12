#include <chrono>
#include <string>

#include "HttpTest.h"
#include "auth/AccessToken.h"

using nlohmann::json;

namespace {

class AuthHttpTest : public HttpTest {
protected:
    Response refresh(const std::string& refreshToken) {
        auto request = makeRequest(drogon::Post, "/api/auth/refresh");
        request->addCookie("refresh_token", refreshToken);
        return send(request);
    }

    // Makes every replaced or logged-out token look as if that happened a minute ago, past the grace period.
    void passTheGracePeriod() {
        server->database->execSqlSync(
            "UPDATE refresh_tokens SET revoked_at = now() - interval '1 minute' WHERE revoked_at IS NOT "
            "NULL");
    }
};

}  // namespace

TEST_F(AuthHttpTest, SignUpLogInAndSeeYourself) {
    const Response signup =
        send(drogon::Post, "/api/auth/signup", {{"email", "  Ada@Example.COM "}, {"password", password}});
    ASSERT_EQ(signup.status, 201);
    EXPECT_EQ(signup.body["email"], "ada@example.com");  // trimmed and lowercased
    EXPECT_FALSE(signup.body.contains("passwordHash"));

    const Response login =
        send(drogon::Post, "/api/auth/login", {{"email", "ada@example.com"}, {"password", password}});
    ASSERT_EQ(login.status, 200);
    EXPECT_EQ(login.body["tokenType"], "Bearer");
    EXPECT_EQ(login.body["expiresIn"], 900);
    const drogon::Cookie& cookie = login.raw->getCookie("refresh_token");
    EXPECT_FALSE(cookie.value().empty());
    EXPECT_TRUE(cookie.isHttpOnly());
    EXPECT_TRUE(cookie.isSecure());
    EXPECT_EQ(cookie.path(), "/api/auth");

    const Response me = send(drogon::Get, "/api/me", nullptr, login.body["accessToken"]);
    ASSERT_EQ(me.status, 200);
    EXPECT_EQ(me.body["id"], signup.body["id"]);
    EXPECT_EQ(me.body["email"], "ada@example.com");
}

TEST_F(AuthHttpTest, SignupRejectsBadInput) {
    auto notJson = makeRequest(drogon::Post, "/api/auth/signup");
    notJson->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    notJson->setBody("{not json");
    EXPECT_EQ(send(notJson).body["error"]["code"], "bad_request");

    EXPECT_EQ(send(drogon::Post, "/api/auth/signup", {{"email", "no-at-sign"}, {"password", password}})
                  .body["error"]["code"],
              "invalid_email");
    EXPECT_EQ(send(drogon::Post, "/api/auth/signup", {{"email", "a@example.com"}, {"password", "short"}})
                  .body["error"]["code"],
              "invalid_password");

    ASSERT_EQ(
        send(drogon::Post, "/api/auth/signup", {{"email", "a@example.com"}, {"password", password}}).status,
        201);
    const Response taken =
        send(drogon::Post, "/api/auth/signup", {{"email", "A@example.com"}, {"password", password}});
    EXPECT_EQ(taken.status, 409);
    EXPECT_EQ(taken.body["error"]["code"], "email_taken");
}

TEST_F(AuthHttpTest, AWrongPasswordAndAnUnknownEmailGetTheSameAnswer) {
    signUpAndLogIn("a@example.com");

    const Response wrongPassword =
        send(drogon::Post, "/api/auth/login", {{"email", "a@example.com"}, {"password", "not the password"}});
    const Response unknownEmail =
        send(drogon::Post, "/api/auth/login", {{"email", "nobody@example.com"}, {"password", password}});

    EXPECT_EQ(wrongPassword.status, 401);
    EXPECT_EQ(wrongPassword.body, unknownEmail.body);
    EXPECT_EQ(unknownEmail.status, 401);
}

TEST_F(AuthHttpTest, RefreshGivesNewTokensThatWork) {
    const Login login = signUpAndLogIn("a@example.com");

    const Response refreshed = refresh(login.refreshToken);

    ASSERT_EQ(refreshed.status, 200);
    const std::string newRefreshToken = refreshed.raw->getCookie("refresh_token").value();
    EXPECT_NE(newRefreshToken, login.refreshToken);
    EXPECT_EQ(send(drogon::Get, "/api/me", nullptr, refreshed.body["accessToken"]).status, 200);
    EXPECT_EQ(refresh(newRefreshToken).status, 200);
}

TEST_F(AuthHttpTest, ARefreshTokenUsedTwiceAtOnceDoesNotLogOut) {
    const Login login = signUpAndLogIn("a@example.com");
    const Response first = refresh(login.refreshToken);
    ASSERT_EQ(first.status, 200);

    // Another tab refreshing with the same token at the same moment.
    const Response second = refresh(login.refreshToken);

    EXPECT_EQ(second.status, 401);
    EXPECT_EQ(refresh(first.raw->getCookie("refresh_token").value()).status, 200);  // still logged in
}

TEST_F(AuthHttpTest, ReusingAReplacedRefreshTokenLogsOutThatLogin) {
    const Login login = signUpAndLogIn("a@example.com");
    const Response refreshed = refresh(login.refreshToken);
    ASSERT_EQ(refreshed.status, 200);
    passTheGracePeriod();

    const Response reused = refresh(login.refreshToken);  // a thief replaying an old token

    EXPECT_EQ(reused.status, 401);
    EXPECT_EQ(reused.body["error"]["code"], "invalid_refresh_token");
    EXPECT_EQ(refresh(refreshed.raw->getCookie("refresh_token").value()).status, 401);  // revoked too
}

TEST_F(AuthHttpTest, LogoutEndsTheRefreshToken) {
    const Login login = signUpAndLogIn("a@example.com");
    auto request = makeRequest(drogon::Post, "/api/auth/logout");
    request->addCookie("refresh_token", login.refreshToken);

    const Response logout = send(request);

    EXPECT_EQ(logout.status, 204);
    EXPECT_EQ(logout.raw->getCookie("refresh_token").maxAge(), 0);  // the browser deletes it
    EXPECT_EQ(refresh(login.refreshToken).status, 401);
}

TEST_F(AuthHttpTest, BadAccessTokensAreRefused) {
    const Login login = signUpAndLogIn("a@example.com");
    const auto past =
        std::chrono::system_clock::now() - vec::AccessTokens::lifetime - std::chrono::minutes(1);

    const Response expired =
        send(drogon::Get, "/api/me", nullptr, vec::AccessTokens(server->jwtSecret).issue(login.userId, past));
    EXPECT_EQ(expired.status, 401);
    EXPECT_EQ(expired.body["error"]["code"], "token_expired");
    EXPECT_EQ(expired.raw->getHeader("WWW-Authenticate"), "Bearer");

    const std::string forged = vec::AccessTokens(std::string(32, 'x')).issue(login.userId);
    for (const std::string& token : {forged, std::string("garbage"), login.accessToken + "x"}) {
        const Response refused = send(drogon::Get, "/api/me", nullptr, token);
        EXPECT_EQ(refused.status, 401) << token;
        EXPECT_EQ(refused.body["error"]["code"], "unauthorized") << token;
    }
}

TEST_F(AuthHttpTest, TheDemoUserLogsInWithoutAPasswordOnceItExists) {
    // Named, but not created yet: nothing to offer, or to log in as.
    EXPECT_EQ(send(drogon::Get, "/api/auth/demo").body["error"]["code"], "no_demo");
    EXPECT_EQ(send(drogon::Post, "/api/auth/demo").body["error"]["code"], "no_demo");

    signUpAndLogIn(demoEmail);
    const Response offered = send(drogon::Get, "/api/auth/demo");
    ASSERT_EQ(offered.status, 200);
    EXPECT_EQ(offered.body["email"], demoEmail);
    const Response demo = send(drogon::Post, "/api/auth/demo");
    ASSERT_EQ(demo.status, 200);
    EXPECT_FALSE(demo.raw->getCookie("refresh_token").value().empty());  // a login like any other
    const Response me = send(drogon::Get, "/api/me", nullptr, demo.body["accessToken"]);
    EXPECT_EQ(me.body["email"], demoEmail);
}
