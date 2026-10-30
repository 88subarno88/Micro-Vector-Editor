#ifndef OPERATION_H
#define OPERATION_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "core/Geometry.h"
#include "core/GraphicsObject.h"

namespace vec {

// The geometry of each kind of shape. RectGeom covers both rectangle kinds.
struct CircleGeom {
    Point center;
    double radius = 0.0;
};

struct LineGeom {
    Point start;
    Point end;
};

struct HexagonGeom {
    Point center;
    double radius = 0.0;
};

struct FreehandGeom {
    std::vector<Point> points;
};

struct TextData {
    Point position;
    std::string text;
    double fontSize = 16.0;
};

// Describes any shape as plain data, with no id and no GraphicsObject.
struct ShapeData {
    std::variant<RectGeom, CircleGeom, LineGeom, HexagonGeom, FreehandGeom, TextData> geometry;
    Style style;
};

// One edit to a Diagram, as plain data that can be stored, sent to other clients and inverted.
struct AddShape {
    ShapeId id;
    ShapeData shape;
};

struct DeleteShape {
    ShapeId id;
};

struct Move {
    ShapeId id;
    double dx = 0.0;
    double dy = 0.0;
};

struct Resize {
    ShapeId id;
    double factor = 1.0;
};

// Only the fields that are set change.
struct SetStyle {
    ShapeId id;
    std::optional<std::string> stroke;
    std::optional<std::string> fill;
    std::optional<double> strokeWidth;
};

using Operation = std::variant<AddShape, DeleteShape, Move, Resize, SetStyle>;

}  // namespace vec

#endif // OPERATION_H
