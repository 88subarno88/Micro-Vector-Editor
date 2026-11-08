#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include "HttpTest.h"

using nlohmann::json;

namespace {

class DocHttpTest : public HttpTest {
protected:
    void SetUp() override {
        HttpTest::SetUp();
        if (IsSkipped()) return;
        token = signUpAndLogIn("owner@example.com").accessToken;
    }

    Response create(const std::string& title) {
        return send(drogon::Post, "/api/docs", {{"title", title}}, token);
    }

    Response importSvg(const std::string& svg, const std::string& query = "") {
        auto request = makeRequest(drogon::Post, "/api/docs" + query, nullptr, token);
        request->setContentTypeString("image/svg+xml");
        request->setBody(svg);
        return send(request);
    }

    static std::string fixture(const std::string& name) {
        std::ifstream in(std::string(FIXTURE_DIR) + "/" + name);
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }

    std::string token;
};

}  // namespace

TEST_F(DocHttpTest, CreateReadRenameAndDeleteADocument) {
    const Response created = create("  Plans  ");
    ASSERT_EQ(created.status, 201);
    const std::string id = created.body["id"];
    EXPECT_EQ(created.body["title"], "Plans");  // trimmed
    EXPECT_EQ(created.body["headSeq"], 0);

    const Response read = send(drogon::Get, "/api/docs/" + id, nullptr, token);
    ASSERT_EQ(read.status, 200);
    EXPECT_EQ(read.body["document"]["title"], "Plans");
    EXPECT_EQ(read.body["role"], "owner");
    EXPECT_EQ(read.body["snapshot"], nullptr);
    EXPECT_EQ(read.body["ops"], json::array());
    EXPECT_EQ(read.body["opsComplete"], true);

    const Response renamed = send(drogon::Patch, "/api/docs/" + id, {{"title", "Final plans"}}, token);
    ASSERT_EQ(renamed.status, 200);
    EXPECT_EQ(renamed.body["title"], "Final plans");
    EXPECT_EQ(send(drogon::Get, "/api/docs/" + id, nullptr, token).body["document"]["title"], "Final plans");

    EXPECT_EQ(send(drogon::Delete, "/api/docs/" + id, nullptr, token).status, 204);
    EXPECT_EQ(send(drogon::Get, "/api/docs/" + id, nullptr, token).status, 404);
}

TEST_F(DocHttpTest, BadTitlesAreRejected) {
    for (const json& body : {json{{"title", ""}}, json{{"title", "   "}}, json{{"title", "a\nb"}},
                             json{{"title", std::string(201, 'a')}}}) {
        const Response response = send(drogon::Post, "/api/docs", body, token);
        EXPECT_EQ(response.status, 400) << body;
        EXPECT_EQ(response.body["error"]["code"], "invalid_title") << body;
    }
    for (const json& body : {json{{"name", "x"}}, json{{"title", 5}}, json::array()}) {
        EXPECT_EQ(send(drogon::Post, "/api/docs", body, token).body["error"]["code"], "bad_request") << body;
    }
}

TEST_F(DocHttpTest, TheListPagesThroughYourDocumentsNewestFirst) {
    std::vector<std::string> created;
    for (int i = 0; i < 5; ++i) created.push_back(create("doc " + std::to_string(i)).body["id"]);
    const std::string otherToken = signUpAndLogIn("other@example.com").accessToken;
    send(drogon::Post, "/api/docs", {{"title", "not yours"}}, otherToken);

    std::vector<std::string> listed;
    std::string cursor;
    int pages = 0;
    do {
        const Response page = send(
            drogon::Get, "/api/docs?limit=2" + (cursor.empty() ? "" : "&cursor=" + cursor), nullptr, token);
        ASSERT_EQ(page.status, 200);
        for (const json& doc : page.body["documents"]) listed.push_back(doc["id"]);
        cursor = page.body["nextCursor"].is_null() ? "" : page.body["nextCursor"].get<std::string>();
        ++pages;
    } while (!cursor.empty() && pages < 10);

    EXPECT_EQ(listed, std::vector<std::string>(created.rbegin(), created.rend()));
    EXPECT_EQ(pages, 3);
}

TEST_F(DocHttpTest, BadListParametersAreRejected) {
    EXPECT_EQ(send(drogon::Get, "/api/docs?limit=many", nullptr, token).body["error"]["code"], "bad_request");
    EXPECT_EQ(send(drogon::Get, "/api/docs?cursor=nonsense", nullptr, token).body["error"]["code"],
              "invalid_cursor");
}

TEST_F(DocHttpTest, AnImportedSvgBecomesTheFirstSnapshot) {
    const Response created = importSvg(fixture("rect.svg"), "?title=Imported");
    ASSERT_EQ(created.status, 201);
    EXPECT_EQ(created.body["title"], "Imported");

    const Response read =
        send(drogon::Get, "/api/docs/" + created.body["id"].get<std::string>(), nullptr, token);
    ASSERT_EQ(read.status, 200);
    EXPECT_EQ(read.body["snapshot"]["seq"], 0);
    EXPECT_EQ(read.body["snapshot"]["state"]["shapes"].size(), 2u);
}

TEST_F(DocHttpTest, AnSvgThatDoesNotParseIsRejectedWithWhere) {
    const Response response = importSvg(fixture("malformed.svg"));

    EXPECT_EQ(response.status, 400);
    EXPECT_EQ(response.body["error"]["code"], "invalid_svg");
    EXPECT_NE(response.body["error"]["message"].get<std::string>().find("line 6"), std::string::npos)
        << response.body;
}

TEST_F(DocHttpTest, ABodyOverOneMegabyteIsRefused) {
    EXPECT_EQ(importSvg(std::string(1024 * 1024 + 1, ' ')).status, 413);
}
