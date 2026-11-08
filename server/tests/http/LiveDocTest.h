#ifndef SERVER_LIVE_DOC_TEST_H
#define SERVER_LIVE_DOC_TEST_H

#include <drogon/utils/coroutine.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "DocClient.h"
#include "HttpTest.h"
#include "collab/LoadDrawing.h"
#include "ops/Json.h"

// Base for tests with live connections to one document, which check that every client ends up holding
// the drawing the server stored.
class LiveDocTest : public HttpTest {
protected:
    // Creates the document as `owner`, who is signed up and logged in, and makes each of `editors` (signed
    // up and logged in too) an editor of it.
    void createSharedDocument(const std::string& owner, const std::vector<std::string>& editors) {
        ownerToken = signUpAndLogIn(owner).accessToken;
        documentId = send(drogon::Post, "/api/docs", {{"title", "shared"}}, ownerToken).body["id"];
        for (const std::string& editor : editors) {
            editorTokens.push_back(signUpAndLogIn(editor).accessToken);
            ASSERT_EQ(send(drogon::Post, "/api/docs/" + documentId + "/members",
                           {{"email", editor}, {"role", "editor"}}, ownerToken)
                          .status,
                      201);
        }
    }

    std::unique_ptr<DocClient> connect(const std::string& token) {
        auto client = std::make_unique<DocClient>(server->port, documentId, token);
        EXPECT_TRUE(client->joined()) << client->problem();
        return client;
    }

    int64_t headSeq() {
        return send(drogon::Get, "/api/docs/" + documentId, nullptr, ownerToken).body["document"]["headSeq"];
    }

    // The drawing as the server would load it now, from Postgres.
    nlohmann::json serverDrawing() {
        const auto drawing = drogon::sync_wait(vec::loadDrawing(server->database, documentId));
        EXPECT_TRUE(drawing) << drawing.error();
        return drawing ? vec::diagramToJson(drawing->diagram) : nlohmann::json();
    }

    // Waits until every client has its answers and has heard of every committed op, then checks they all
    // hold the drawing the server would load.
    void expectEveryoneAgrees(std::initializer_list<DocClient*> clients) {
        // First every answer, so that the head read next is final.
        for (DocClient* client : clients) ASSERT_TRUE(client->waitUntilSettled(0)) << "unanswered ops";
        const int64_t head = headSeq();
        const nlohmann::json expected = serverDrawing();
        for (DocClient* client : clients) {
            ASSERT_TRUE(client->waitUntilSettled(head))
                << "stuck at seq " << client->lastSeq() << " of " << head;
            EXPECT_EQ(client->problem(), "");
            EXPECT_EQ(client->replica(), expected);
        }
    }

    std::string documentId;
    std::string ownerToken;
    std::vector<std::string> editorTokens;
};

#endif  // SERVER_LIVE_DOC_TEST_H
