#ifndef SERVER_LOAD_DRAWING_H
#define SERVER_LOAD_DRAWING_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <cstdint>
#include <string>
#include <tl/expected.hpp>

#include "core/Diagram.h"

namespace vec {

struct LoadedDrawing {
    Diagram diagram;
    int64_t lastSeq = 0;  // the seq of the last op applied
};

// A document's current drawing: its newest snapshot with every later op applied, read in batches so that
// a long log is never held in memory at once. The error says why the stored data couldn't be used, such
// as an op that no longer applies. Database failures throw DrogonDbException.
drogon::Task<tl::expected<LoadedDrawing, std::string>> loadDrawing(drogon::orm::DbClientPtr database,
                                                                   std::string documentId);

}  // namespace vec

#endif  // SERVER_LOAD_DRAWING_H
