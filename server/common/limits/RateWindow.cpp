#include "limits/RateWindow.h"

namespace vec {

RateWindow rateWindow(std::string_view scope, std::string_view client,
                      std::chrono::system_clock::time_point now, std::chrono::seconds length) {
    const int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const int64_t window = seconds / length.count();
    return {"ratelimit:" + std::string(scope) + ":" + std::string(client) + ":" + std::to_string(window),
            length.count() - seconds % length.count()};
}

}  // namespace vec
