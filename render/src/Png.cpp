#include "render/Png.h"

#include <lunasvg/lunasvg.h>

#include <algorithm>
#include <cstdint>

namespace vec {

namespace {

// lunasvg has no system fonts of its own, and without one it silently leaves text out.
bool loadFallbackFont() {
    for (const char* path : {"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                             "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"}) {
        if (lunasvg_add_font_face_from_file("", false, false, path)) return true;
    }
    return false;
}

bool hasText(const Diagram& diagram) {
    const auto& shapes = diagram.getShapes();
    return std::any_of(shapes.begin(), shapes.end(),
                       [](const auto& shape) { return shape->getType() == "Text"; });
}

void append(void* closure, void* data, int size) {
    static_cast<std::string*>(closure)->append(static_cast<const char*>(data), static_cast<size_t>(size));
}

}  // namespace

tl::expected<PngImage, std::string> renderPng(const Diagram& diagram) {
    static const bool haveFont = loadFallbackFont();
    if (!haveFont && hasText(diagram)) {
        return tl::unexpected(
            std::string("cannot draw text: no font found (install fonts-liberation or fonts-dejavu-core)"));
    }

    constexpr uint32_t opaqueWhite = 0xFFFFFFFF;
    const auto document = lunasvg::Document::loadFromData(diagram.toSVG());
    const lunasvg::Bitmap bitmap =
        document ? document->renderToBitmap(-1, -1, opaqueWhite) : lunasvg::Bitmap();
    PngImage png{{}, bitmap.width(), bitmap.height()};
    if (bitmap.isNull() || !bitmap.writeToPng(append, &png.bytes)) {
        return tl::unexpected(std::string("cannot render the drawing"));
    }
    return png;
}

}  // namespace vec
