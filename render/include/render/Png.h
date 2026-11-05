#ifndef VEC_RENDER_PNG_H
#define VEC_RENDER_PNG_H

#include <string>
#include <tl/expected.hpp>

#include "core/Diagram.h"

namespace vec {

struct PngImage {
    std::string bytes;  // a whole PNG file
    int width = 0;
    int height = 0;
};

// The drawing as a PNG file, at its canvas size on white. Text needs Liberation Sans or DejaVu
// Sans installed; without either, a drawing with text is an error rather than an image missing its text.
// Not thread-safe.
tl::expected<PngImage, std::string> renderPng(const Diagram& diagram);

}  // namespace vec

#endif  // VEC_RENDER_PNG_H
