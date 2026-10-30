#include "ops/Apply.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "core/Circle.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"

namespace vec {

namespace {

using Result = tl::expected<void, ApplyError>;

// Shapes write these values inside "..." in toSVG, so a quote would end the attribute early.
bool isSafeAttribute(const std::string& value) {
    return value.find('"') == std::string::npos;
}

// Builds the shape described by each kind of geometry.
struct ShapeBuilder {
    std::unique_ptr<GraphicsObject> operator()(const RectGeom& g) const {
        if (g.cornerRadius > 0) {
            return std::make_unique<RoundedRectangle>(g.x, g.y, g.width, g.height, g.cornerRadius);
        }
        return std::make_unique<Rectangle>(g.x, g.y, g.width, g.height);
    }
    std::unique_ptr<GraphicsObject> operator()(const CircleGeom& g) const {
        return std::make_unique<Circle>(g.center.x, g.center.y, g.radius);
    }
    std::unique_ptr<GraphicsObject> operator()(const LineGeom& g) const {
        return std::make_unique<Line>(g.start.x, g.start.y, g.end.x, g.end.y);
    }
    std::unique_ptr<GraphicsObject> operator()(const HexagonGeom& g) const {
        return std::make_unique<Hexagon>(g.center.x, g.center.y, g.radius);
    }
    std::unique_ptr<GraphicsObject> operator()(const FreehandGeom& g) const {
        auto sketch = std::make_unique<FreehandSketch>();
        for (const Point& p : g.points) sketch->addPoint(p.x, p.y);
        return sketch;
    }
    std::unique_ptr<GraphicsObject> operator()(const TextData& g) const {
        auto text = std::make_unique<Text>(g.position.x, g.position.y, g.text);
        text->setFrontSize(g.fontSize);
        return text;
    }
};

struct Applier {
    Diagram& diagram;

    Result operator()(const AddShape& op) const {
        if (diagram.find(op.id)) return tl::unexpected(ApplyError::DuplicateId);
        if (const auto* text = std::get_if<TextData>(&op.shape.geometry); text && text->fontSize <= 0) {
            return tl::unexpected(ApplyError::InvalidShape);  // Text would throw
        }
        if (!isSafeAttribute(op.id) || !isSafeAttribute(op.shape.style.stroke) ||
            !isSafeAttribute(op.shape.style.fill)) {
            return tl::unexpected(ApplyError::InvalidShape);
        }
        if (op.position && *op.position > static_cast<size_t>(diagram.getObjectCount())) {
            return tl::unexpected(ApplyError::InvalidShape);
        }

        auto shape = std::visit(ShapeBuilder{}, op.shape.geometry);
        shape->setId(op.id);
        shape->setStrokeColor(op.shape.style.stroke);
        shape->setFillColor(op.shape.style.fill);
        shape->setStrokeWidth(op.shape.style.strokeWidth);
        diagram.insertObject(std::move(shape), op.position.value_or(diagram.getObjectCount()));
        return {};
    }

    Result operator()(const DeleteShape& op) const {
        if (!diagram.removeObject(op.id)) return tl::unexpected(ApplyError::ShapeNotFound);
        return {};
    }

    Result operator()(const Move& op) const {
        if (!std::isfinite(op.dx) || !std::isfinite(op.dy)) return tl::unexpected(ApplyError::InvalidValue);

        GraphicsObject* shape = diagram.find(op.id);
        if (!shape) return tl::unexpected(ApplyError::ShapeNotFound);
        shape->move(op.dx, op.dy);
        return {};
    }

    Result operator()(const Resize& op) const {
        // 0 can't be undone and a negative factor would flip the shape
        if (!std::isfinite(op.factor) || op.factor <= 0) return tl::unexpected(ApplyError::InvalidValue);

        GraphicsObject* shape = diagram.find(op.id);
        if (!shape) return tl::unexpected(ApplyError::ShapeNotFound);
        shape->scale_factor(op.factor);
        return {};
    }

    Result operator()(const SetStyle& op) const {
        // checked first, so a bad field can't leave the others half applied
        if ((op.stroke && !isSafeAttribute(*op.stroke)) || (op.fill && !isSafeAttribute(*op.fill)) ||
            (op.strokeWidth && (!std::isfinite(*op.strokeWidth) || *op.strokeWidth < 0))) {
            return tl::unexpected(ApplyError::InvalidValue);
        }

        GraphicsObject* shape = diagram.find(op.id);
        if (!shape) return tl::unexpected(ApplyError::ShapeNotFound);
        if (op.stroke) shape->setStrokeColor(*op.stroke);
        if (op.fill) shape->setFillColor(*op.fill);
        if (op.strokeWidth) shape->setStrokeWidth(*op.strokeWidth);
        return {};
    }
};

struct Inverter {
    const Diagram& diagram;
    using InverseResult = tl::expected<Operation, ApplyError>;

    InverseResult operator()(const AddShape& op) const {
        if (diagram.find(op.id)) return tl::unexpected(ApplyError::DuplicateId);
        return DeleteShape{op.id};
    }

    InverseResult operator()(const DeleteShape& op) const {
        const GraphicsObject* shape = diagram.find(op.id);
        if (!shape) return tl::unexpected(ApplyError::ShapeNotFound);
        return AddShape{op.id, toShapeData(*shape), diagram.positionOf(op.id)};
    }

    InverseResult operator()(const Move& op) const {
        if (!diagram.find(op.id)) return tl::unexpected(ApplyError::ShapeNotFound);
        return Move{op.id, -op.dx, -op.dy};
    }

    InverseResult operator()(const Resize& op) const {
        const GraphicsObject* shape = diagram.find(op.id);
        if (!shape) return tl::unexpected(ApplyError::ShapeNotFound);

        double inverse = 1.0 / op.factor;
        if (const auto* text = dynamic_cast<const Text*>(shape)) {
            // Text stops shrinking at font size 1, so go back from where it will really end up
            inverse = text->getFontSize() / std::max(text->getFontSize() * op.factor, 1.0);
        }
        // a factor so small that 1/factor overflows could never be undone
        if (!std::isfinite(op.factor) || op.factor <= 0 || !std::isfinite(inverse)) {
            return tl::unexpected(ApplyError::InvalidValue);
        }
        return Resize{op.id, inverse};
    }

    InverseResult operator()(const SetStyle& op) const {
        const GraphicsObject* shape = diagram.find(op.id);
        if (!shape) return tl::unexpected(ApplyError::ShapeNotFound);

        SetStyle inverse{op.id, std::nullopt, std::nullopt, std::nullopt};
        if (op.stroke) inverse.stroke = shape->getStrokeColor();
        if (op.fill) inverse.fill = shape->getFillColor();
        if (op.strokeWidth) inverse.strokeWidth = shape->getStrokeWidth();
        return inverse;
    }
};

}  // namespace

ShapeData toShapeData(const GraphicsObject& shape) {
    const Style style{shape.getStrokeColor(), shape.getFillColor(), shape.getStrokeWidth()};

    if (const auto* s = dynamic_cast<const RoundedRectangle*>(&shape)) {
        return {RectGeom{s->getX(), s->getY(), s->getWidth(), s->getHeight(), s->getCornerRadius()}, style};
    }
    if (const auto* s = dynamic_cast<const Circle*>(&shape)) {
        return {CircleGeom{{s->getCenterX(), s->getCenterY()}, s->getRadius()}, style};
    }
    if (const auto* s = dynamic_cast<const Line*>(&shape)) {
        return {LineGeom{{s->getX1(), s->getY1()}, {s->getX2(), s->getY2()}}, style};
    }
    if (const auto* s = dynamic_cast<const Hexagon*>(&shape)) {
        return {HexagonGeom{{s->getCenterX(), s->getCenterY()}, s->getRadius()}, style};
    }
    if (const auto* s = dynamic_cast<const FreehandSketch*>(&shape)) {
        return {FreehandGeom{s->getPoints()}, style};
    }
    if (const auto* s = dynamic_cast<const Text*>(&shape)) {
        return {TextData{{s->getX(), s->getY()}, s->getText(), s->getFontSize()}, style};
    }
    return {RectGeom{shape.getX(), shape.getY(), shape.getWidth(), shape.getHeight()}, style};
}

Result apply(Diagram& diagram, const Operation& op) {
    return std::visit(Applier{diagram}, op);
}

tl::expected<Operation, ApplyError> invert(const Diagram& diagram, const Operation& op) {
    return std::visit(Inverter{diagram}, op);
}

}  // namespace vec
