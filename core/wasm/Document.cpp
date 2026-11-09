#include "Document.h"

#include <nlohmann/json.hpp>
#include <utility>

#include "core/Renderer.h"
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

json styleJson(const Style& style) {
    return {{"stroke", style.stroke}, {"fill", style.fill}, {"strokeWidth", style.strokeWidth}};
}

// Writes down what a shape draws, for JavaScript to replay on a canvas.
class RecordingRenderer : public Renderer {
public:
    json commands = json::array();
    std::string shapeId;

    void drawRect(const RectGeom& r, const Style& style) override {
        add("rect", style,
            {{"x", r.x},
             {"y", r.y},
             {"width", r.width},
             {"height", r.height},
             {"cornerRadius", r.cornerRadius}});
    }
    void drawEllipse(const EllipseGeom& e, const Style& style) override {
        add("ellipse", style, {{"cx", e.center.x}, {"cy", e.center.y}, {"rx", e.rx}, {"ry", e.ry}});
    }
    void drawPolyline(const std::vector<Point>& points, bool closed, const Style& style) override {
        json flat = json::array();
        for (const Point& p : points) {
            flat.push_back(p.x);
            flat.push_back(p.y);
        }
        add("polyline", style, {{"points", std::move(flat)}, {"closed", closed}});
    }
    void drawText(const TextGeom& t, const Style& style) override {
        add("text", style,
            {{"x", t.topLeft.x},
             {"y", t.topLeft.y},
             {"text", t.text},
             {"fontFamily", t.fontFamily},
             {"fontSize", t.fontSize}});
    }

private:
    void add(const char* kind, const Style& style, json fields) {
        fields["id"] = shapeId;
        fields["kind"] = kind;
        fields["style"] = styleJson(style);
        commands.push_back(std::move(fields));
    }
};

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

std::string Document::boundsOf(const std::string& id) const {
    const GraphicsObject* shape = diagram_.find(id);
    if (!shape) return "null";
    return dump({{"x", shape->getX()},
                 {"y", shape->getY()},
                 {"width", shape->getWidth()},
                 {"height", shape->getHeight()}});
}

std::string Document::translatedShape(const std::string& shapeJson, double dx, double dy) const {
    // Decoded as an addShape, so it's checked exactly as one would be, then moved by the shape's own code.
    const json shape = json::parse(shapeJson, nullptr, false);
    const auto op = decodeOperation(json{{"kind", "addShape"}, {"id", "copy"}, {"shape", shape}});
    if (!op) return error("bad_shape", op.error().message);
    Diagram scratch;
    if (const auto added = apply(scratch, *op); !added) {
        return error("bad_shape", toString(added.error()));
    }
    if (const auto moved = apply(scratch, Move{"copy", dx, dy}); !moved) {
        return error("bad_shape", toString(moved.error()));
    }
    return dump({{"shape", toShapeData(*scratch.find("copy"))}});
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

std::string Document::drawCommands() const {
    RecordingRenderer recorder;
    for (const auto& shape : diagram_.getShapes()) {
        recorder.shapeId = shape->getId();
        shape->draw(recorder);
    }
    return dump(recorder.commands);
}

}  // namespace vec::web
