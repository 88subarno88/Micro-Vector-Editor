#include "parser/SVGParser.h"
#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

//include all shapes
#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Line.h"
#include "core/Hexagon.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"
#include "core/FreehandSketch.h"

enum class WhatdaShape {
    Rect,
    Circle,
    Line,
    Text,
    Hexagon,
    Freehand,
    Unknown
};

namespace {

// Maps a tag such as "<rect x=..."/> to its shape kind by exact tag name.
WhatdaShape identifyTag(std::string_view tag) {
    static const std::pair<std::string_view, WhatdaShape> tags[] = {
        {"rect", WhatdaShape::Rect},       {"circle", WhatdaShape::Circle},
        {"line", WhatdaShape::Line},       {"text", WhatdaShape::Text},
        {"polygon", WhatdaShape::Hexagon}, {"path", WhatdaShape::Freehand},
    };
    tag.remove_prefix(1);  // '<'
    const std::string_view name = tag.substr(0, tag.find_first_of(" \t\r\n/>"));
    for (const auto& [tagName, kind] : tags) {
        if (name == tagName) return kind;
    }
    return WhatdaShape::Unknown;
}

// Sets the stroke colour and width present in the tag; missing ones keep the shape's defaults.
void applyStroke(GraphicsObject& shape, const SVGParser::Attributes& attrs) {
    if (const auto it = attrs.find("stroke"); it != attrs.end()) shape.setStrokeColor(it->second);
    if (const auto it = attrs.find("stroke-width"); it != attrs.end()) {
        shape.setStrokeWidth(SVGParser::parseNumber(it->second, shape.getStrokeWidth()));
    }
}

void applyFill(GraphicsObject& shape, const SVGParser::Attributes& attrs) {
    if (const auto it = attrs.find("fill"); it != attrs.end()) shape.setFillColor(it->second);
}

}  // namespace

//parse shapes
void SVGParser::parse_Tag(std::string_view tagText, std::string_view content, Diagram& diagram) {
    const WhatdaShape tag = identifyTag(tagText);
    if (tag == WhatdaShape::Unknown) return;

    const Attributes attrs = parseAttributes(tagText);
    const auto text = [&](const char* key) {
        const auto it = attrs.find(key);
        return it == attrs.end() ? std::string() : it->second;
    };
    const auto number = [&](const char* key) { return parseNumber(text(key), 0.0); };
    // Keeps the file's id, unless it's empty, taken, or would break the quotes when saved;
    // the shape's generated id stays in those cases.
    const auto add = [&](std::unique_ptr<GraphicsObject> shape) {
        const std::string id = text("id");
        if (!id.empty() && id.find('"') == std::string::npos && !diagram.find(id)) shape->setId(id);
        diagram.addObject(std::move(shape));
    };

    switch (tag) {
        case WhatdaShape::Rect: {
            const double x = number("x"), y = number("y");
            const double w = number("width"), h = number("height");
            const double rx = number("rx");   // rx > 0 means a rounded rectangle

            std::unique_ptr<GraphicsObject> shape;
            if (rx > 0) shape = std::make_unique<RoundedRectangle>(x, y, w, h, rx);
            else        shape = std::make_unique<Rectangle>(x, y, w, h);
            applyStroke(*shape, attrs);
            applyFill(*shape, attrs);
            add(std::move(shape));
            break;
        }

        case WhatdaShape::Circle: {
            auto shape = std::make_unique<Circle>(number("cx"), number("cy"), number("r"));
            applyStroke(*shape, attrs);
            applyFill(*shape, attrs);
            add(std::move(shape));
            break;
        }

        case WhatdaShape::Line: {
            auto shape = std::make_unique<Line>(number("x1"), number("y1"), number("x2"), number("y2"));
            applyStroke(*shape, attrs);
            add(std::move(shape));
            break;
        }

        case WhatdaShape::Text:{
            if (!content.empty()) {
                auto shape = std::make_unique<Text>(number("x"), number("y"), decodeEntities(content));
                if (const double size = number("font-size"); size > 0) shape->setFrontSize(size);
                applyStroke(*shape, attrs);
                applyFill(*shape, attrs);
                add(std::move(shape));
            }
            break;
        }

        case WhatdaShape::Hexagon: {
            // "x1,y1 x2,y2 ..." with any mix of commas and whitespace.
            std::string points = text("points");
            std::replace(points.begin(), points.end(), ',', ' ');
            std::istringstream ss(points);
            std::vector<double> coords;
            for (double value; ss >> value;) coords.push_back(value);
            if (coords.size() < 2) break;  // no complete x,y pair

            double minX = coords[0], maxX = coords[0], minY = coords[1], maxY = coords[1];
            for (size_t i = 0; i + 1 < coords.size(); i += 2) {  // a trailing lone value is ignored
                minX = std::min(minX, coords[i]);
                maxX = std::max(maxX, coords[i]);
                minY = std::min(minY, coords[i + 1]);
                maxY = std::max(maxY, coords[i + 1]);
            }

            auto shape = std::make_unique<Hexagon>((minX + maxX) / 2.0, (minY + maxY) / 2.0,
                                                   (maxX - minX) / 2.0);
            applyStroke(*shape, attrs);
            applyFill(*shape, attrs);
            add(std::move(shape));
            break;
        }

        case WhatdaShape::Freehand: {
            auto sketch = std::make_unique<FreehandSketch>();
            std::stringstream ss(text("d"));
            char cmd;
            double px, py;

            while (ss >> cmd) {
                if (cmd == 'M' || cmd == 'L') {
                    if (ss >> px >> py) {
                        sketch->addPoint(px, py);
                    }
                }
            }
            applyStroke(*sketch, attrs);
            add(std::move(sketch));
            break;
        }

        case WhatdaShape::Unknown:
            break;
    }
}
