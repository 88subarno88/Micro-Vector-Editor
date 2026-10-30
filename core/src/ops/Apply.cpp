#include "ops/Apply.h"

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

        auto shape = std::visit(ShapeBuilder{}, op.shape.geometry);
        shape->setId(op.id);
        shape->setStrokeColor(op.shape.style.stroke);
        shape->setFillColor(op.shape.style.fill);
        shape->setStrokeWidth(op.shape.style.strokeWidth);
        diagram.addObject(std::move(shape));
        return {};
    }

    Result operator()(const DeleteShape& op) const {
        if (!diagram.removeObject(op.id)) return tl::unexpected(ApplyError::ShapeNotFound);
        return {};
    }

    template <typename Op>
    Result operator()(const Op&) const {
        return tl::unexpected(ApplyError::Unsupported);
    }
};

}  // namespace

Result apply(Diagram& diagram, const Operation& op) {
    return std::visit(Applier{diagram}, op);
}

}  // namespace vec
