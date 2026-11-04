#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "ops/Apply.h"
#include "ops/Json.h"
#include "parser/SVGParser.h"

using namespace vec;

namespace {

// One shape of every kind, for the ops to act on. The fuzzer learns these ids from the dictionary.
Diagram startingDiagram() {
    Diagram diagram;
    const Style style{"red", "blue", 3};
    for (const AddShape& op : {AddShape{"s_1", {RectGeom{1, 2, 30, 40}, style}, std::nullopt},
                               AddShape{"s_2", {RectGeom{1, 2, 30, 40, 5}, style}, std::nullopt},
                               AddShape{"s_3", {CircleGeom{{1, 2}, 3}, style}, std::nullopt},
                               AddShape{"s_4", {LineGeom{{1, 2}, {3, 4}}, style}, std::nullopt},
                               AddShape{"s_5", {HexagonGeom{{1, 2}, 3}, style}, std::nullopt},
                               AddShape{"s_6", {FreehandGeom{{{1, 2}, {3, 4}}}, style}, std::nullopt},
                               AddShape{"s_7", {TextData{{1, 2}, "Hi", 20}, style}, std::nullopt}}) {
        if (!vec::apply(diagram, Operation{op})) std::abort();
    }
    return diagram;
}

// Whatever a diagram holds, it must survive being saved as SVG and as JSON.
void checkSaves(const Diagram& diagram) {
    int written = 0;  // a sketch with no points writes nothing
    for (const auto& shape : diagram.getShapes()) {
        if (!shape->toSVG().empty()) ++written;
    }
    const auto fromSvg = SVGParser::parseSVG(diagram.toSVG());
    if (!fromSvg || fromSvg->getObjectCount() != written) std::abort();

    const std::string text = diagramToJson(diagram).dump();
    if (!decodeDiagram(std::string_view(text))) std::abort();
}

}  // namespace

// Feeds arbitrary bytes to the decoders. A crash or sanitizer report is a bug. So is an op that
// doesn't survive re-encoding, an op that applies but can't be undone, or a diagram left unsaveable.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const std::string_view text(reinterpret_cast<const char*>(data), size);

    if (const auto snapshot = decodeDiagram(text)) checkSaves(*snapshot);

    const auto op = decodeOperation(text);
    if (!op) return 0;

    const nlohmann::json encoded = *op;
    const auto decoded = decodeOperation(std::string_view(encoded.dump()));
    if (!decoded || nlohmann::json(*decoded) != encoded) std::abort();

    Diagram diagram = startingDiagram();
    // The server checks an op before storing it, so a check that disagrees with apply would store bad ops.
    const auto checked = check(diagram, *op);
    Diagram applied = startingDiagram();
    if (checked.has_value() != vec::apply(applied, *op).has_value()) std::abort();
    const auto inverse = invert(diagram, *op);
    if (!inverse) return 0;
    if (!vec::apply(diagram, *op)) return 0;  // a rejected op is fine
    checkSaves(diagram);

    if (!vec::apply(diagram, *inverse)) std::abort();
    checkSaves(diagram);
    return 0;
}
