#include "collab/LoadDrawing.h"

#include "db/OpRepo.h"
#include "ops/Apply.h"
#include "ops/Json.h"

namespace vec {

drogon::Task<tl::expected<LoadedDrawing, std::string>> loadDrawing(drogon::orm::DbClientPtr database,
                                                                   std::string documentId) {
    LoadedDrawing loaded;
    if (const auto snapshot = co_await SnapshotRepo(database).latest(documentId)) {
        auto diagram = decodeDiagram(snapshot->state);
        if (!diagram)
            co_return tl::unexpected("snapshot " + std::to_string(snapshot->seq) + ": " +
                                     diagram.error().message);
        loaded.diagram = std::move(*diagram);
        loaded.lastSeq = snapshot->seq;
    }

    OpRepo ops(database);
    while (true) {
        const auto batch = co_await ops.opsAfter(documentId, loaded.lastSeq);
        for (const OpRow& row : batch) {
            const auto op = decodeOperation(row.payload);
            if (!op) co_return tl::unexpected("op " + std::to_string(row.seq) + ": " + op.error().message);
            if (!vec::apply(loaded.diagram, *op))
                co_return tl::unexpected("op " + std::to_string(row.seq) + " doesn't apply");
            loaded.lastSeq = row.seq;
        }
        if (static_cast<int>(batch.size()) < OpRepo::maxOpsPerRead) break;
    }
    co_return loaded;
}

}  // namespace vec
