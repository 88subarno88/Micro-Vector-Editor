#include "ops/Json.h"

#include "ops/Apply.h"

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using nlohmann::json;

namespace vec {

namespace {

json pointToJson(const Point& p) {
    return {{"x", p.x}, {"y", p.y}};
}

Point pointFromJson(const json& j) {
    return {j.at("x").get<double>(), j.at("y").get<double>()};
}

// Each kind of geometry as the JSON fields of its shape.
struct GeometryEncoder {
    json operator()(const RectGeom& g) const {
        return {{"kind", "rect"}, {"x", g.x}, {"y", g.y}, {"width", g.width},
                {"height", g.height}, {"cornerRadius", g.cornerRadius}};
    }
    json operator()(const CircleGeom& g) const {
        return {{"kind", "circle"}, {"center", pointToJson(g.center)}, {"radius", g.radius}};
    }
    json operator()(const LineGeom& g) const {
        return {{"kind", "line"}, {"start", pointToJson(g.start)}, {"end", pointToJson(g.end)}};
    }
    json operator()(const HexagonGeom& g) const {
        return {{"kind", "hexagon"}, {"center", pointToJson(g.center)}, {"radius", g.radius}};
    }
    json operator()(const FreehandGeom& g) const {
        json points = json::array();
        for (const Point& p : g.points) points.push_back(pointToJson(p));
        return {{"kind", "freehand"}, {"points", points}};
    }
    json operator()(const TextData& g) const {
        return {{"kind", "text"}, {"position", pointToJson(g.position)}, {"text", g.text},
                {"fontSize", g.fontSize}};
    }
};

struct OperationEncoder {
    json operator()(const AddShape& op) const {
        json j = {{"kind", "addShape"}, {"id", op.id}, {"shape", op.shape}};
        if (op.position) j["position"] = *op.position;
        return j;
    }
    json operator()(const DeleteShape& op) const {
        return {{"kind", "deleteShape"}, {"id", op.id}};
    }
    json operator()(const Move& op) const {
        return {{"kind", "move"}, {"id", op.id}, {"dx", op.dx}, {"dy", op.dy}};
    }
    json operator()(const Resize& op) const {
        return {{"kind", "resize"}, {"id", op.id}, {"factor", op.factor}};
    }
    json operator()(const SetStyle& op) const {
        json j = {{"kind", "setStyle"}, {"id", op.id}};
        if (op.stroke) j["stroke"] = *op.stroke;
        if (op.fill) j["fill"] = *op.fill;
        if (op.strokeWidth) j["strokeWidth"] = *op.strokeWidth;
        return j;
    }
};

constexpr int snapshotVersion = 1;

// The value of an optional field; nullopt if it's left out.
template <typename T>
std::optional<T> optionalField(const json& j, const char* key) {
    const auto it = j.find(key);
    if (it == j.end()) return std::nullopt;
    return it->get<T>();
}

}  // namespace

void to_json(json& j, const ShapeData& shape) {
    j = std::visit(GeometryEncoder{}, shape.geometry);
    j["style"] = {{"stroke", shape.style.stroke},
                  {"fill", shape.style.fill},
                  {"strokeWidth", shape.style.strokeWidth}};
}

void from_json(const json& j, ShapeData& shape) {
    const std::string kind = j.at("kind").get<std::string>();
    if (kind == "rect") {
        shape.geometry = RectGeom{j.at("x").get<double>(), j.at("y").get<double>(),
                                  j.at("width").get<double>(), j.at("height").get<double>(),
                                  j.at("cornerRadius").get<double>()};
    } else if (kind == "circle") {
        shape.geometry = CircleGeom{pointFromJson(j.at("center")), j.at("radius").get<double>()};
    } else if (kind == "line") {
        shape.geometry = LineGeom{pointFromJson(j.at("start")), pointFromJson(j.at("end"))};
    } else if (kind == "hexagon") {
        shape.geometry = HexagonGeom{pointFromJson(j.at("center")), j.at("radius").get<double>()};
    } else if (kind == "freehand") {
        std::vector<Point> points;
        if (!j.at("points").is_array()) throw std::invalid_argument("points must be an array");
        for (const json& point : j.at("points")) points.push_back(pointFromJson(point));
        shape.geometry = FreehandGeom{std::move(points)};
    } else if (kind == "text") {
        shape.geometry = TextData{pointFromJson(j.at("position")), j.at("text").get<std::string>(),
                                  j.at("fontSize").get<double>()};
    } else {
        throw std::invalid_argument("unknown shape kind: " + kind);
    }

    const json& style = j.at("style");
    shape.style = {style.at("stroke").get<std::string>(), style.at("fill").get<std::string>(),
                   style.at("strokeWidth").get<double>()};
}

void to_json(json& j, const Operation& op) {
    j = std::visit(OperationEncoder{}, op);
}

void from_json(const json& j, Operation& op) {
    const std::string kind = j.at("kind").get<std::string>();
    const ShapeId id = j.at("id").get<ShapeId>();
    if (id.empty()) throw std::invalid_argument("id must not be empty");
    if (kind == "addShape") {
        // get<size_t> alone would accept -1, 1.5 and true
        if (j.contains("position") && !j.at("position").is_number_unsigned()) {
            throw std::invalid_argument("position must be a non-negative integer");
        }
        op = AddShape{id, j.at("shape").get<ShapeData>(), optionalField<size_t>(j, "position")};
    } else if (kind == "deleteShape") {
        op = DeleteShape{id};
    } else if (kind == "move") {
        op = Move{id, j.at("dx").get<double>(), j.at("dy").get<double>()};
    } else if (kind == "resize") {
        op = Resize{id, j.at("factor").get<double>()};
    } else if (kind == "setStyle") {
        op = SetStyle{id, optionalField<std::string>(j, "stroke"), optionalField<std::string>(j, "fill"),
                      optionalField<double>(j, "strokeWidth")};
    } else {
        throw std::invalid_argument("unknown operation kind: " + kind);
    }
}

tl::expected<Operation, DecodeError> decodeOperation(const json& j) {
    try {
        return j.get<Operation>();
    } catch (const std::exception& e) {
        return tl::unexpected(DecodeError{e.what()});
    }
}

tl::expected<Operation, DecodeError> decodeOperation(std::string_view text) {
    try {
        return decodeOperation(json::parse(text));
    } catch (const std::exception& e) {
        return tl::unexpected(DecodeError{e.what()});
    }
}

json diagramToJson(const Diagram& diagram) {
    json shapes = json::array();
    for (const auto& shape : diagram.getShapes()) {
        shapes.push_back({{"id", shape->getId()}, {"shape", toShapeData(*shape)}});
    }
    return {{"version", snapshotVersion}, {"shapes", shapes}};
}

tl::expected<Diagram, DecodeError> decodeDiagram(const json& j) {
    try {
        if (j.at("version") != snapshotVersion) {
            return tl::unexpected(DecodeError{"unsupported snapshot version"});
        }
        if (!j.at("shapes").is_array()) return tl::unexpected(DecodeError{"shapes must be an array"});

        Diagram diagram;
        for (const json& entry : j.at("shapes")) {
            const ShapeId id = entry.at("id").get<ShapeId>();
            if (id.empty()) return tl::unexpected(DecodeError{"id must not be empty"});

            const Operation add = AddShape{id, entry.at("shape").get<ShapeData>(), std::nullopt};
            if (!vec::apply(diagram, add)) return tl::unexpected(DecodeError{"invalid shape: " + id});
        }
        return diagram;
    } catch (const std::exception& e) {
        return tl::unexpected(DecodeError{e.what()});
    }
}

tl::expected<Diagram, DecodeError> decodeDiagram(std::string_view text) {
    try {
        return decodeDiagram(json::parse(text));
    } catch (const std::exception& e) {
        return tl::unexpected(DecodeError{e.what()});
    }
}

}  // namespace vec
