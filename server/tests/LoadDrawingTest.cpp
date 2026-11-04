#include <string>

#include "DatabaseTest.h"
#include "collab/LoadDrawing.h"
#include "db/DocRepo.h"
#include "db/OpRepo.h"
#include "ops/Apply.h"
#include "ops/Json.h"

using namespace vec;
using nlohmann::json;

namespace {

class LoadDrawingTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped()) return;
        user = newUser();
        document = drogon::sync_wait(DocRepo(database).create(user.id, "doc"));
    }

    void addOp(int64_t seq, const json& op) {
        database->execSqlSync(
            "INSERT INTO operations (document_id, seq, user_id, client_op_id, payload) "
            "VALUES ($1::uuid, $2::bigint, $3::uuid, $4, $5::jsonb)",
            document.id, seq, user.id, "op-" + std::to_string(seq), op.dump());
    }

    json addRect(const std::string& id, double x) {
        return json(Operation{AddShape{id, {RectGeom{x, 0, 10, 10}, {}}, std::nullopt}});
    }

    tl::expected<LoadedDrawing, std::string> load() {
        return drogon::sync_wait(loadDrawing(database, document.id));
    }

    User user;
    Document document;
};

}  // namespace

TEST_F(LoadDrawingTest, ANewDocumentIsEmpty) {
    const auto loaded = load();

    ASSERT_TRUE(loaded) << loaded.error();
    EXPECT_EQ(loaded->lastSeq, 0);
    EXPECT_EQ(loaded->diagram.getObjectCount(), 0);
}

TEST_F(LoadDrawingTest, OpsAloneAreReplayed) {
    addOp(1, addRect("a", 0));
    addOp(2, addRect("b", 5));
    addOp(3, json(Operation{Move{"a", 100, 0}}));

    const auto loaded = load();

    ASSERT_TRUE(loaded) << loaded.error();
    EXPECT_EQ(loaded->lastSeq, 3);
    EXPECT_EQ(loaded->diagram.getObjectCount(), 2);
    EXPECT_DOUBLE_EQ(loaded->diagram.find("a")->getX(), 100);
}

TEST_F(LoadDrawingTest, StartsFromTheSnapshotAndAppliesOnlyLaterOps) {
    Diagram snapshot;
    ASSERT_TRUE(vec::apply(snapshot, AddShape{"a", {RectGeom{0, 0, 10, 10}, {}}, std::nullopt}));
    ASSERT_TRUE(drogon::sync_wait(SnapshotRepo(database).insert(document.id, 2, diagramToJson(snapshot))));
    addOp(1, addRect("ignored-1", 0));  // already in the snapshot
    addOp(2, addRect("ignored-2", 0));
    addOp(3, json(Operation{Move{"a", 7, 0}}));

    const auto loaded = load();

    ASSERT_TRUE(loaded) << loaded.error();
    EXPECT_EQ(loaded->lastSeq, 3);
    EXPECT_EQ(loaded->diagram.getObjectCount(), 1);
    EXPECT_DOUBLE_EQ(loaded->diagram.find("a")->getX(), 7);
}

TEST_F(LoadDrawingTest, ReadsALogLongerThanOneBatch) {
    addOp(1, addRect("a", 0));
    database->execSqlSync(
        "INSERT INTO operations (document_id, seq, user_id, client_op_id, payload) "
        "SELECT $1::uuid, s, $2::uuid, 'op-' || s, '{\"kind\":\"move\",\"id\":\"a\",\"dx\":1,\"dy\":0}' "
        "FROM generate_series(2, 2501) s",
        document.id, user.id);

    const auto loaded = load();

    ASSERT_TRUE(loaded) << loaded.error();
    EXPECT_EQ(loaded->lastSeq, 2501);
    EXPECT_DOUBLE_EQ(loaded->diagram.find("a")->getX(), 2500);
}

TEST_F(LoadDrawingTest, AnOpThatNoLongerAppliesIsAnError) {
    addOp(1, json(Operation{Move{"missing", 1, 1}}));

    const auto loaded = load();

    ASSERT_FALSE(loaded);
    EXPECT_NE(loaded.error().find("op 1"), std::string::npos) << loaded.error();
}

TEST_F(LoadDrawingTest, AnUndecodableOpIsAnError) {
    addOp(1, json{{"kind", "explode"}, {"id", "a"}});

    EXPECT_FALSE(load());
}
