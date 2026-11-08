#include <chrono>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "LiveDocTest.h"
#include "config/Config.h"
#include "ops/Apply.h"

using namespace vec;
using nlohmann::json;

namespace {

constexpr int clientCount = 3;
constexpr int opsPerClient = 200;
// Each connection may send 30 ops a second on average; stay under it, so that no op is rate limited.
constexpr int opsPerBatch = 25;
constexpr std::chrono::milliseconds batchInterval{1000};

// Random edits to a handful of shape IDs, so that clients often touch the same shapes: moving one that
// another has just deleted, adding one that already exists, and so on.
class RandomOps {
public:
    explicit RandomOps(uint32_t seed) : rng_(seed) {}

    Operation next() {
        const ShapeId target = "s" + std::to_string(integer(0, 7));
        switch (integer(0, 9)) {
            case 0:
            case 1:
            case 2:
                return AddShape{target, shape(), std::nullopt};
            case 3:
                return DeleteShape{target};
            case 4:
            case 5:
                return Move{target, real(-50, 50), real(-50, 50)};
            case 6:
                return Resize{target, real(0.8, 1.25)};
            default:
                return SetStyle{target, maybe(color()), maybe(color()),
                                integer(0, 1) ? std::optional<double>(real(0.5, 5)) : std::nullopt};
        }
    }

private:
    int integer(int low, int high) { return std::uniform_int_distribution<int>(low, high)(rng_); }
    double real(double low, double high) { return std::uniform_real_distribution<double>(low, high)(rng_); }
    Point point() { return {real(0, 800), real(0, 600)}; }

    std::string color() {
        static const std::vector<std::string> colors = {"red", "navy", "none", "#00ff00", "#123abc", "black"};
        return colors[integer(0, static_cast<int>(colors.size()) - 1)];
    }
    std::optional<std::string> maybe(const std::string& value) {
        return integer(0, 1) ? std::optional<std::string>(value) : std::nullopt;
    }

    ShapeData shape() {
        const Style style{color(), color(), real(0.5, 5)};
        switch (integer(0, 5)) {
            case 0:
                return {RectGeom{real(0, 700), real(0, 500), real(1, 100), real(1, 100), real(0, 10)}, style};
            case 1:
                return {CircleGeom{point(), real(1, 80)}, style};
            case 2:
                return {LineGeom{point(), point()}, style};
            case 3:
                return {HexagonGeom{point(), real(1, 80)}, style};
            case 4: {
                std::vector<Point> points;
                for (int i = integer(2, 6); i > 0; --i) points.push_back(point());
                return {FreehandGeom{points}, style};
            }
            default:
                return {TextData{point(), "note " + std::to_string(integer(0, 999)), real(8, 40)}, style};
        }
    }

    std::mt19937 rng_;
};

// A seed from VEC_TEST_SEED to replay a failure, otherwise a fresh one each run.
uint32_t chooseSeed() {
    const auto fixed = readEnv("VEC_TEST_SEED");
    return fixed ? static_cast<uint32_t>(std::stoul(*fixed)) : std::random_device{}();
}

class PropertyHttpTest : public LiveDocTest {
protected:
    void SetUp() override {
        LiveDocTest::SetUp();
        if (IsSkipped()) return;
        createSharedDocument("owner@example.com", {"editor1@example.com", "editor2@example.com"});
        clients.push_back(connect(ownerToken));
        for (const std::string& token : editorTokens) clients.push_back(connect(token));
    }

    // The drawing as a client reading over REST would build it: the newest snapshot plus the ops after it.
    json restDrawing() {
        const json doc = send(drogon::Get, "/api/docs/" + documentId, nullptr, ownerToken).body;
        EXPECT_TRUE(doc["opsComplete"]);
        auto diagram = doc["snapshot"].is_null() ? tl::expected<Diagram, DecodeError>(Diagram())
                                                 : decodeDiagram(doc["snapshot"]["state"]);
        if (!diagram) {
            ADD_FAILURE() << "undecodable snapshot";
            return nullptr;
        }
        for (const json& op : doc["ops"]) {
            const auto decoded = decodeOperation(op["op"]);
            if (!decoded || !apply(*diagram, *decoded))
                ADD_FAILURE() << "op " << op["seq"] << " didn't apply";
        }
        return diagramToJson(*diagram);
    }

    std::vector<std::unique_ptr<DocClient>> clients;
};

}  // namespace

TEST_F(PropertyHttpTest, EveryReplicaEndsUpWithTheServersDrawing) {
    const uint32_t seed = chooseSeed();
    SCOPED_TRACE("rerun with VEC_TEST_SEED=" + std::to_string(seed));
    RecordProperty("seed", static_cast<int>(seed));
    std::vector<RandomOps> generators;
    for (int i = 0; i < clientCount; ++i) generators.emplace_back(seed + i);

    // In batches, each client's ops interleaved with the others', so the server sees them all at once.
    for (int sent = 0; sent < opsPerClient; sent += opsPerBatch) {
        for (int i = 0; i < opsPerBatch && sent + i < opsPerClient; ++i) {
            for (int c = 0; c < clientCount; ++c) clients[c]->send(generators[c].next());
        }
        std::this_thread::sleep_for(batchInterval);
    }

    std::vector<DocClient*> all;
    for (const auto& client : clients) all.push_back(client.get());
    expectEveryoneAgrees({all[0], all[1], all[2]});

    int acked = 0;
    for (DocClient* client : all) {
        EXPECT_EQ(client->acked() + client->rejected(), opsPerClient);
        acked += client->acked();
        // Ops may conflict, but none should fail for reasons other than the drawing.
        for (const char* reason : {"rate_limited", "unavailable", "bad_op", "forbidden"}) {
            EXPECT_EQ(client->rejectedFor().count(reason), 0u) << reason;
        }
    }
    EXPECT_EQ(headSeq(), acked);
    // Over 200, so that the server snapshotted and the room no longer holds the whole log.
    ASSERT_GT(acked, 200) << "too few ops applied to test snapshots";

    // More than 200 ops, so the server has taken snapshots: a REST reader and a new client, which now
    // joins from a snapshot rather than the whole log, must see the same drawing too.
    EXPECT_EQ(restDrawing(), serverDrawing());
    const auto late = connect(ownerToken);
    expectEveryoneAgrees({late.get()});
}
