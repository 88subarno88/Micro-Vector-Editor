#include <string>
#include <vector>

#include "HttpTest.h"

using nlohmann::json;

namespace {

// One document, owned by "owner", shared with an editor and a viewer; "stranger" isn't a member.
class PermissionHttpTest : public HttpTest {
protected:
    void SetUp() override {
        HttpTest::SetUp();
        if (IsSkipped()) return;
        owner = signUpAndLogIn("owner@example.com").accessToken;
        editor = signUpAndLogIn("editor@example.com").accessToken;
        viewer = signUpAndLogIn("viewer@example.com").accessToken;
        stranger = signUpAndLogIn("stranger@example.com").accessToken;
        doc = "/api/docs/" +
              send(drogon::Post, "/api/docs", {{"title", "shared"}}, owner).body["id"].get<std::string>();
        ASSERT_EQ(share("editor@example.com", "editor").status, 201);
        ASSERT_EQ(share("viewer@example.com", "viewer").status, 201);
    }

    Response share(const std::string& email, const std::string& role) {
        return send(drogon::Post, doc + "/members", {{"email", email}, {"role", role}}, owner);
    }

    struct Call {
        drogon::HttpMethod method;
        std::string path;
        json body;
    };

    // Every route on the document, in an order that leaves it in place: deleting comes last.
    std::vector<Call> everyCall() const {
        return {{drogon::Get, doc, nullptr},
                {drogon::Get, doc + "/ops", nullptr},
                {drogon::Get, doc + "/versions/0", nullptr},
                {drogon::Patch, doc, {{"title", "renamed"}}},
                {drogon::Post, doc + "/members", {{"email", "stranger@example.com"}, {"role", "viewer"}}},
                {drogon::Post, doc + "/exports", {{"format", "svg"}}},
                {drogon::Delete, doc, nullptr}};
    }

    // The status each of everyCall() gets with `token`.
    std::vector<int> statusesFor(const std::string& token) {
        std::vector<int> statuses;
        for (const Call& call : everyCall())
            statuses.push_back(send(call.method, call.path, call.body, token).status);
        return statuses;
    }

    std::string owner, editor, viewer, stranger, doc;
};

}  // namespace

// Get, history, a version, rename, share, export, delete.
TEST_F(PermissionHttpTest, WithoutATokenEveryRouteIs401) {
    EXPECT_EQ(statusesFor(""), (std::vector<int>{401, 401, 401, 401, 401, 401, 401}));
}

TEST_F(PermissionHttpTest, ANonMemberGets404EverywhereAsIfTheDocumentDidNotExist) {
    EXPECT_EQ(statusesFor(stranger), (std::vector<int>{404, 404, 404, 404, 404, 404, 404}));
    // The same answer as for a document that really doesn't exist.
    EXPECT_EQ(send(drogon::Get, doc, nullptr, stranger).body,
              send(drogon::Get, "/api/docs/00000000-0000-0000-0000-000000000000", nullptr, stranger).body);
    EXPECT_EQ(send(drogon::Get, "/api/docs/not-a-uuid", nullptr, stranger).status, 404);
}

TEST_F(PermissionHttpTest, AViewerCanReadAndExportOnly) {
    EXPECT_EQ(statusesFor(viewer), (std::vector<int>{200, 200, 200, 403, 403, 202, 403}));
    EXPECT_EQ(send(drogon::Patch, doc, {{"title", "x"}}, viewer).body["error"]["code"], "forbidden");
}

TEST_F(PermissionHttpTest, AnEditorCanAlsoRenameButNotShareOrDelete) {
    EXPECT_EQ(statusesFor(editor), (std::vector<int>{200, 200, 200, 200, 403, 202, 403}));
}

TEST_F(PermissionHttpTest, TheOwnerCanDoEverything) {
    EXPECT_EQ(statusesFor(owner), (std::vector<int>{200, 200, 200, 200, 201, 202, 204}));
}

TEST_F(PermissionHttpTest, EachMemberSeesTheirOwnRole) {
    for (const auto& [token, role] : {std::pair{owner, "owner"}, {editor, "editor"}, {viewer, "viewer"}}) {
        EXPECT_EQ(send(drogon::Get, doc, nullptr, token).body["role"], role);
    }
}

TEST_F(PermissionHttpTest, SharingRefusesOwnershipUnknownUsersAndExistingMembers) {
    EXPECT_EQ(share("stranger@example.com", "owner").body["error"]["code"], "invalid_role");
    EXPECT_EQ(share("nobody@example.com", "viewer").body["error"]["code"], "user_not_found");

    const Response again = share("viewer@example.com", "editor");
    EXPECT_EQ(again.status, 409);
    EXPECT_EQ(again.body["error"]["code"], "already_member");
    EXPECT_EQ(send(drogon::Get, doc, nullptr, viewer).body["role"], "viewer");  // unchanged
}

TEST_F(PermissionHttpTest, ANewMemberCanSeeTheDocumentInTheirList) {
    ASSERT_EQ(share("stranger@example.com", "viewer").status, 201);

    const Response list = send(drogon::Get, "/api/docs", nullptr, stranger);

    ASSERT_EQ(list.body["documents"].size(), 1u);
    EXPECT_EQ("/api/docs/" + list.body["documents"][0]["id"].get<std::string>(), doc);
}

TEST_F(PermissionHttpTest, ExportsAreHiddenFromNonMembers) {
    const Response job = send(drogon::Post, doc + "/exports", {{"format", "png"}}, viewer);
    ASSERT_EQ(job.status, 202);
    const std::string path = "/api/exports/" + job.body["id"].get<std::string>();

    EXPECT_EQ(send(drogon::Get, path, nullptr, owner).status, 200);
    EXPECT_EQ(send(drogon::Get, path, nullptr, stranger).status, 404);
    EXPECT_EQ(send(drogon::Get, path + "/file", nullptr, stranger).status, 404);
    EXPECT_EQ(send(drogon::Get, path + "/file", nullptr, owner).status, 409);  // not exported yet
}
