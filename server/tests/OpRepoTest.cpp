#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "DatabaseTest.h"
#include "db/DocRepo.h"
#include "db/OpRepo.h"

using namespace vec;
using nlohmann::json;

namespace {

const json movePayload = {{"kind", "move"}, {"id", "s_1"}, {"dx", 1.5}, {"dy", -2}};

class OpRepoTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        user = newUser();
        document = drogon::sync_wait(DocRepo(database).create(user.id, "doc"));
    }

    // The whole write path, in one transaction, on `client`.
    drogon::Task<InsertedOp> append(drogon::orm::DbClientPtr client, std::string clientOpId,
                                    json payload = movePayload) {
        auto transaction = co_await client->newTransactionCoro();
        const auto head = co_await OpRepo::lockHead(transaction, document.id);
        const InsertedOp inserted =
            co_await OpRepo::insert(transaction, document.id, *head, user.id, clientOpId, payload);
        EXPECT_TRUE(co_await commit(std::move(transaction)));
        co_return inserted;
    }

    InsertedOp appendNow(const std::string& clientOpId) {
        return drogon::sync_wait(append(database, clientOpId));
    }

    int64_t headSeq() { return drogon::sync_wait(DocRepo(database).get(document.id))->headSeq; }

    std::vector<OpRow> opsAfter(int64_t seq, int limit = OpRepo::maxOpsPerRead) {
        return drogon::sync_wait(OpRepo(database).opsAfter(document.id, seq, limit));
    }

    User user;
    Document document;
};

}  // namespace

TEST_F(OpRepoTest, InsertAssignsConsecutiveSeqsAndAdvancesTheHead) {
    EXPECT_EQ(appendNow("a").seq, 1);
    EXPECT_EQ(appendNow("b").seq, 2);
    EXPECT_EQ(appendNow("c").seq, 3);
    EXPECT_EQ(headSeq(), 3);
}

TEST_F(OpRepoTest, ACommittedOpIsVisibleToOtherConnectionsAtOnce) {
    const auto pool = drogon::orm::DbClient::newPgClient(std::getenv("TEST_DATABASE_URL"), 2);
    new drogon::orm::DbClientPtr(pool);  // never destroyed, for the reason in DatabaseTest

    for (int i = 1; i <= 50; ++i) {
        drogon::sync_wait(append(pool, std::to_string(i)));
        ASSERT_EQ(headSeq(), i);  // read on a different client
    }
}

TEST_F(OpRepoTest, InsertingUpdatesTheDocumentsUpdatedAt) {
    appendNow("a");

    EXPECT_GT(drogon::sync_wait(DocRepo(database).get(document.id))->updatedAt, document.updatedAt);
}

TEST_F(OpRepoTest, ARetryReturnsTheOriginalSeqAndChangesNothing) {
    appendNow("a");
    appendNow("b");

    const InsertedOp retry = appendNow("a");

    EXPECT_TRUE(retry.duplicate);
    EXPECT_EQ(retry.seq, 1);
    EXPECT_EQ(headSeq(), 2);
    EXPECT_EQ(opsAfter(0).size(), 2u);
}

TEST_F(OpRepoTest, TheNextOpAfterARetryHasNoGap) {
    appendNow("a");
    appendNow("a");

    EXPECT_EQ(appendNow("b").seq, 2);
}

TEST_F(OpRepoTest, ClientOpIdsAreScopedToADocument) {
    appendNow("a");
    const Document other = drogon::sync_wait(DocRepo(database).create(user.id, "other"));

    const auto insertInOther = [&]() -> drogon::Task<InsertedOp> {
        auto transaction = co_await database->newTransactionCoro();
        const auto head = co_await OpRepo::lockHead(transaction, other.id);
        const InsertedOp result =
            co_await OpRepo::insert(transaction, other.id, *head, user.id, "a", movePayload);
        co_await commit(std::move(transaction));
        co_return result;
    };
    const InsertedOp inserted = drogon::sync_wait(insertInOther());

    EXPECT_FALSE(inserted.duplicate);
    EXPECT_EQ(inserted.seq, 1);
}

TEST_F(OpRepoTest, ARolledBackInsertLeavesNoTrace) {
    const auto rollBack = [&]() -> drogon::Task<> {
        const auto transaction = co_await database->newTransactionCoro();
        const auto head = co_await OpRepo::lockHead(transaction, document.id);
        co_await OpRepo::insert(transaction, document.id, *head, user.id, "a", movePayload);
        transaction->rollback();
    };
    drogon::sync_wait(rollBack());

    EXPECT_EQ(headSeq(), 0);
    EXPECT_TRUE(opsAfter(0).empty());
    EXPECT_EQ(appendNow("a").seq, 1);
}

TEST_F(OpRepoTest, LockHeadOfAnUnknownDocumentIsNothing) {
    const auto lockUnknown = [&](std::string id) -> drogon::Task<std::optional<int64_t>> {
        const auto transaction = co_await database->newTransactionCoro();
        co_return co_await OpRepo::lockHead(transaction, id);
    };

    EXPECT_EQ(drogon::sync_wait(lockUnknown("00000000-0000-0000-0000-000000000000")), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(lockUnknown("not-a-uuid")), std::nullopt);
}

TEST_F(OpRepoTest, ConcurrentWritersAreSerialized) {
    constexpr int threads = 6;
    constexpr int opsPerThread = 15;
    const auto pool = drogon::orm::DbClient::newPgClient(std::getenv("TEST_DATABASE_URL"), threads);
    // Never destroyed, for the reason in DatabaseTest.
    new drogon::orm::DbClientPtr(pool);

    std::vector<std::thread> writers;
    for (int t = 0; t < threads; ++t) {
        writers.emplace_back([&, t] {
            for (int i = 0; i < opsPerThread; ++i) {
                drogon::sync_wait(append(pool, std::to_string(t) + "-" + std::to_string(i)));
            }
        });
    }
    for (std::thread& writer : writers) writer.join();

    const auto ops = opsAfter(0);
    ASSERT_EQ(ops.size(), threads * opsPerThread);
    EXPECT_EQ(headSeq(), threads * opsPerThread);
    for (size_t i = 0; i < ops.size(); ++i) EXPECT_EQ(ops[i].seq, static_cast<int64_t>(i) + 1);
}

TEST_F(OpRepoTest, ConcurrentRetriesOfOneOpApplyOnce) {
    constexpr int threads = 6;
    const auto pool = drogon::orm::DbClient::newPgClient(std::getenv("TEST_DATABASE_URL"), threads);
    new drogon::orm::DbClientPtr(pool);

    std::vector<std::thread> retries;
    for (int t = 0; t < threads; ++t) {
        retries.emplace_back([&] { EXPECT_EQ(drogon::sync_wait(append(pool, "same")).seq, 1); });
    }
    for (std::thread& retry : retries) retry.join();

    EXPECT_EQ(headSeq(), 1);
    EXPECT_EQ(opsAfter(0).size(), 1u);
}

TEST_F(OpRepoTest, OpsAfterReadsARowBackInOrder) {
    appendNow("a");
    appendNow("b");

    const auto ops = opsAfter(0);

    ASSERT_EQ(ops.size(), 2u);
    EXPECT_EQ(ops[0].documentId, document.id);
    EXPECT_EQ(ops[0].seq, 1);
    EXPECT_EQ(ops[0].userId, user.id);
    EXPECT_EQ(ops[0].clientOpId, "a");
    EXPECT_EQ(ops[0].payload, movePayload);
    EXPECT_FALSE(ops[0].createdAt.empty());
    EXPECT_EQ(ops[1].seq, 2);
}

TEST_F(OpRepoTest, OpsAfterSkipsWhatTheCallerHasAndObeysTheLimit) {
    for (const char* id : {"a", "b", "c", "d"}) appendNow(id);

    const auto tail = opsAfter(2);
    ASSERT_EQ(tail.size(), 2u);
    EXPECT_EQ(tail[0].seq, 3);

    const auto limited = opsAfter(0, 3);
    ASSERT_EQ(limited.size(), 3u);
    EXPECT_EQ(limited[2].seq, 3);

    EXPECT_TRUE(opsAfter(4).empty());
    EXPECT_EQ(opsAfter(0, 0).size(), 1u);
}

TEST_F(OpRepoTest, OpsAfterWithABadIdIsEmpty) {
    EXPECT_TRUE(drogon::sync_wait(OpRepo(database).opsAfter("not-a-uuid", 0)).empty());
}

TEST_F(OpRepoTest, PayloadSurvivesSpecialCharacters) {
    const json payload = {{"text", "quote\" 'single' \\ back\nnewline caf\xc3\xa9 $1 ;DROP"}};
    drogon::sync_wait(append(database, "a", payload));

    EXPECT_EQ(opsAfter(0)[0].payload, payload);
}

TEST_F(OpRepoTest, LatestSnapshotIsTheNewestByLogPosition) {
    SnapshotRepo snapshots(database);
    EXPECT_EQ(drogon::sync_wait(snapshots.latest(document.id)), std::nullopt);
    EXPECT_EQ(drogon::sync_wait(snapshots.latest("not-a-uuid")), std::nullopt);

    EXPECT_TRUE(drogon::sync_wait(snapshots.insert(document.id, 400, {{"at", 400}})));
    EXPECT_TRUE(drogon::sync_wait(snapshots.insert(document.id, 200, {{"at", 200}})));

    const auto latest = drogon::sync_wait(snapshots.latest(document.id));
    ASSERT_TRUE(latest);
    EXPECT_EQ(latest->documentId, document.id);
    EXPECT_EQ(latest->seq, 400);
    EXPECT_EQ(latest->state.at("at"), 400);
}

TEST_F(OpRepoTest, ASecondSnapshotAtTheSameSeqChangesNothing) {
    SnapshotRepo snapshots(database);
    ASSERT_TRUE(drogon::sync_wait(snapshots.insert(document.id, 5, {{"first", true}})));

    EXPECT_FALSE(drogon::sync_wait(snapshots.insert(document.id, 5, {{"first", false}})));

    EXPECT_EQ(drogon::sync_wait(snapshots.latest(document.id))->state.at("first"), true);
}
