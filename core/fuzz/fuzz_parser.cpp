#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "parser/SVGParser.h"

// Feeds arbitrary bytes to the SVG parser. A crash or sanitizer report is a bug, and so is
// a drawing that can't be saved and loaded again.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const std::string_view doc(reinterpret_cast<const char*>(data), size);

    const auto diagram = SVGParser::parse_SVG(doc);
    if (!diagram) return 0;

    int written = 0;  // a sketch with no points writes nothing
    for (const auto& shape : diagram->getShapes()) {
        if (!shape->toSVG().empty()) ++written;
    }

    const auto reloaded = SVGParser::parse_SVG(diagram->toSVG());
    if (!reloaded || reloaded->getObjectCount() != written) std::abort();
    return 0;
}
