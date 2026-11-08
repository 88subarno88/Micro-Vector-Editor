#include "Document.h"

#include <nlohmann/json.hpp>
#include <utility>

#include "ops/Apply.h"
#include "ops/Json.h"
#include "parser/SVGParser.h"

using nlohmann::json;

namespace vec::web {

namespace {

// Dumps without throwing: text that isn't valid UTF-8 is replaced instead.
std::string dump(const json& j) { return j.dump(-1, ' ', false, json::error_handler_t::replace); }

std::string ok() { return R"({"ok":true})"; }

std::string error(const std::string& code, const std::string& message) {
    return dump({{"error", {{"code", code}, {"message", message}}}});
}

}  // namespace

std::string Document::applyOp(const std::string& opJson) {
    const auto op = decodeOperation(std::string_view(opJson));
    if (!op) return error("bad_op", op.error().message);
    if (const auto applied = apply(diagram_, *op); !applied) {
        return error(toString(applied.error()), "the edit doesn't apply to the drawing");
    }
    return ok();
}

std::string Document::invertOp(const std::string& opJson) const {
    const auto op = decodeOperation(std::string_view(opJson));
    if (!op) return error("bad_op", op.error().message);
    const auto inverse = invert(diagram_, *op);
    if (!inverse) return error(toString(inverse.error()), "the edit can't be undone");
    return dump({{"op", *inverse}});
}

std::string Document::hitTest(double x, double y) const {
    const auto& shapes = diagram_.getShapes();
    for (auto shape = shapes.rbegin(); shape != shapes.rend(); ++shape) {
        if ((*shape)->contains(x, y)) return (*shape)->getId();
    }
    return "";
}

std::string Document::toSVG() const { return diagram_.toSVG(); }

std::string Document::loadSVG(const std::string& svg) {
    auto parsed = SVGParser::parseSVG(svg);
    if (!parsed) {
        const ParseError& e = parsed.error();
        return error("invalid_svg", "line " + std::to_string(e.line) + ", column " +
                                        std::to_string(e.column) + ": " + e.message);
    }
    diagram_ = std::move(*parsed);
    return ok();
}

std::string Document::loadJson(const std::string& snapshotJson) {
    auto decoded = decodeDiagram(std::string_view(snapshotJson));
    if (!decoded) return error("bad_snapshot", decoded.error().message);
    diagram_ = std::move(*decoded);
    return ok();
}

std::string Document::shapesJson() const { return dump(diagramToJson(diagram_)); }

}  // namespace vec::web
