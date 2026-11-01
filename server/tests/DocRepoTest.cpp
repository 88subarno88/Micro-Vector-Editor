#include <optional>
#include <set>
#include <string>
#include <vector>

#include "DatabaseTest.h"
#include "db/DocRepo.h"

using namespace vec;

namespace {

class DocRepoTest : public DatabaseTest {
protected:
    Document newDoc(const User& owner, const std::string& title = "doc") {
        return drogon::sync_wait(DocRepo(database).create(owner.id, title));
    }

    std::vector<std::string> listAllIds(const User& user, int pageSize, int* pages = nullptr) {
        DocRepo repo(database);
        std::vector<std::string> ids;
        std::optional<std::string> cursor;
        do {
            const auto page = drogon::sync_wait(repo.list(user.id, pageSize, cursor));
            EXPECT_TRUE(page);
            for (const Document& doc : page->documents) ids.push_back(doc.id);
            cursor = page->nextCursor;
            if (pages != nullptr) ++*pages;
        } while (cursor);
        return ids;
    }
};

const std::string unknownId = "00000000-0000-0000-0000-000000000000";

}  // namespace

TEST_F(DocRepoTest, CreateFillsInColumnsAndMakesTheOwnerAMember) {
    const User owner = newUser();

    const Document doc = newDoc(owner, "My doc");

    EXPECT_TRUE(isUuid(doc.id));
    EXPECT_EQ(doc.ownerId, owner.id);
    EXPECT_EQ(doc.title, "My doc");
    EXPECT_EQ(doc.headSeq, 0);
    EXPECT_EQ(drogon::sync_wait(MemberRepo(database).roleOf(doc.id, owner.id)), Role::Owner);
}

TEST_F(DocRepoTest, GetFindsADocumentButNotAnUnknownOne) {
    const Document doc = newDoc(newUser());
    DocRepo repo(database);

    const auto found = drogon::sync_wait(repo.get(doc.id));
    ASSERT_TRUE(found);
    EXPECT_EQ(found->title, "doc");

    EXPECT_EQ(drogon::sync_wait(repo.get(unknownId)), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(repo.get("not-a-uuid")), std::nullopt);
}

TEST_F(DocRepoTest, RenameChangesTheTitleAndUpdatedAt) {
    const Document doc = newDoc(newUser());
    DocRepo repo(database);

    const auto renamed = drogon::sync_wait(repo.rename(doc.id, "New; DROP TABLE documents; --"));

    ASSERT_TRUE(renamed);
    EXPECT_EQ(renamed->title, "New; DROP TABLE documents; --");
    EXPECT_GT(renamed->updatedAt, doc.updatedAt);
    EXPECT_EQ(drogon::sync_wait(repo.rename(unknownId, "x")), std::nullopt);
}

TEST_F(DocRepoTest, RemoveDeletesTheDocumentAndItsMembers) {
    const User owner = newUser();
    const Document doc = newDoc(owner);
    DocRepo repo(database);

    EXPECT_TRUE(drogon::sync_wait(repo.remove(doc.id)));

    EXPECT_EQ(drogon::sync_wait(repo.get(doc.id)), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(MemberRepo(database).roleOf(doc.id, owner.id)), std::nullopt);
    EXPECT_FALSE(drogon::sync_wait(repo.remove(doc.id)));
    EXPECT_FALSE(drogon::sync_wait(repo.remove("not-a-uuid")));
}

TEST_F(DocRepoTest, ListShowsOnlyTheUsersDocumentsNewestFirst) {
    const User me = newUser("me");
    const User other = newUser("other");
    const Document first = newDoc(me, "first");
    const Document second = newDoc(me, "second");
    newDoc(other, "not mine");

    const auto page = drogon::sync_wait(DocRepo(database).list(me.id, 10));

    ASSERT_TRUE(page);
    ASSERT_EQ(page->documents.size(), 2u);
    EXPECT_EQ(page->documents[0].id, second.id);
    EXPECT_EQ(page->documents[1].id, first.id);
    EXPECT_EQ(page->nextCursor, std::nullopt);
}

TEST_F(DocRepoTest, ListIncludesDocumentsSharedWithTheUser) {
    const User me = newUser("me");
    const Document shared = newDoc(newUser("other"), "shared");
    ASSERT_TRUE(drogon::sync_wait(MemberRepo(database).add(shared.id, me.id, Role::Viewer)));

    const auto page = drogon::sync_wait(DocRepo(database).list(me.id, 10));

    ASSERT_TRUE(page);
    ASSERT_EQ(page->documents.size(), 1u);
    EXPECT_EQ(page->documents[0].id, shared.id);
}

TEST_F(DocRepoTest, PagingVisitsEveryDocumentOnceInOrder) {
    const User me = newUser();
    std::vector<std::string> created;
    for (int i = 0; i < 7; ++i) created.push_back(newDoc(me).id);

    int pages = 0;
    const auto ids = listAllIds(me, 3, &pages);

    EXPECT_EQ(pages, 3);
    EXPECT_EQ(ids, std::vector<std::string>(created.rbegin(), created.rend()));
}

TEST_F(DocRepoTest, AnExactlyFullLastPageHasNoCursor) {
    const User me = newUser();
    for (int i = 0; i < 4; ++i) newDoc(me);

    int pages = 0;
    EXPECT_EQ(listAllIds(me, 2, &pages).size(), 4u);
    EXPECT_EQ(pages, 2);
}

TEST_F(DocRepoTest, DocumentsCreatedInOneTransactionStillPageCorrectly) {
    // The same created_at on every row, so only the id can break the tie.
    const User me = newUser();
    database->execSqlSync(
        "WITH docs AS (INSERT INTO documents (owner_id, title) SELECT $1::uuid, 'doc' FROM "
        "generate_series(1, 5) "
        "RETURNING id, owner_id) "
        "INSERT INTO document_members (document_id, user_id, role) SELECT id, owner_id, 'owner' FROM docs",
        me.id);

    const auto ids = listAllIds(me, 2);

    EXPECT_EQ(ids.size(), 5u);
    EXPECT_EQ(std::set<std::string>(ids.begin(), ids.end()).size(), 5u);
}

TEST_F(DocRepoTest, EditingOrDeletingBetweenPagesKeepsPagingCorrect) {
    const User me = newUser();
    DocRepo repo(database);
    std::vector<Document> docs;
    for (int i = 0; i < 5; ++i) docs.push_back(newDoc(me));

    const auto first = drogon::sync_wait(repo.list(me.id, 2));
    ASSERT_TRUE(first && first->nextCursor);
    ASSERT_TRUE(drogon::sync_wait(repo.rename(docs[0].id, "edited")));    // the oldest, still to come
    ASSERT_TRUE(drogon::sync_wait(repo.remove(first->documents[1].id)));  // the page's last, the cursor

    const auto second = drogon::sync_wait(repo.list(me.id, 10, first->nextCursor));

    ASSERT_TRUE(second);
    ASSERT_EQ(second->documents.size(), 3u);
    EXPECT_EQ(second->documents[0].id, docs[2].id);
    EXPECT_EQ(second->documents[2].title, "edited");
}

TEST_F(DocRepoTest, LimitIsClamped) {
    const User me = newUser();
    for (int i = 0; i < 2; ++i) newDoc(me);
    DocRepo repo(database);

    for (const int limit : {0, -5}) {
        const auto page = drogon::sync_wait(repo.list(me.id, limit));
        ASSERT_TRUE(page);
        EXPECT_EQ(page->documents.size(), 1u) << limit;
        EXPECT_TRUE(page->nextCursor) << limit;
    }
    const auto huge = drogon::sync_wait(repo.list(me.id, 1000000));
    ASSERT_TRUE(huge);
    EXPECT_EQ(huge->documents.size(), 2u);
}

TEST_F(DocRepoTest, BadCursorsAreAnErrorNotADatabaseFailure) {
    const User me = newUser();
    DocRepo repo(database);

    for (const char* cursor : {"", "abc", "123", "123_",
                               "_"
                               "00000000-0000-0000-0000-000000000000",
                               "12x_00000000-0000-0000-0000-000000000000", "123_not-a-uuid",
                               "1;DROP TABLE documents_00000000-0000-0000-0000-000000000000",
                               "99999999999999999999_00000000-0000-0000-0000-000000000000"}) {
        const auto page = drogon::sync_wait(repo.list(me.id, 10, std::string(cursor)));
        ASSERT_FALSE(page) << cursor;
        EXPECT_EQ(page.error(), ListDocsError::BadCursor);
    }
}

TEST_F(DocRepoTest, AddsAMemberAndReportsTheirRole) {
    const Document doc = newDoc(newUser("owner"));
    const User guest = newUser("guest");
    MemberRepo members(database);

    EXPECT_EQ(drogon::sync_wait(members.roleOf(doc.id, guest.id)), std::nullopt);
    EXPECT_TRUE(drogon::sync_wait(members.add(doc.id, guest.id, Role::Editor)));
    EXPECT_EQ(drogon::sync_wait(members.roleOf(doc.id, guest.id)), Role::Editor);
}

TEST_F(DocRepoTest, AddingAnExistingMemberNeverChangesTheirRole) {
    const User owner = newUser();
    const Document doc = newDoc(owner);
    MemberRepo members(database);

    EXPECT_FALSE(drogon::sync_wait(members.add(doc.id, owner.id, Role::Viewer)));

    EXPECT_EQ(drogon::sync_wait(members.roleOf(doc.id, owner.id)), Role::Owner);
}

TEST_F(DocRepoTest, AddingToOrForSomethingMissingIsFalseNotAnError) {
    const User user = newUser();
    const Document doc = newDoc(user);
    MemberRepo members(database);

    EXPECT_FALSE(drogon::sync_wait(members.add(unknownId, user.id, Role::Editor)));
    EXPECT_FALSE(drogon::sync_wait(members.add(doc.id, unknownId, Role::Editor)));
    EXPECT_FALSE(drogon::sync_wait(members.add("bad", user.id, Role::Editor)));
    EXPECT_EQ(drogon::sync_wait(members.roleOf("bad", user.id)), std::nullopt);
}
