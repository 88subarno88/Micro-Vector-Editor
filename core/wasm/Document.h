#ifndef VEC_WEB_DOCUMENT_H
#define VEC_WEB_DOCUMENT_H

#include <string>

#include "core/Diagram.h"

namespace vec::web {

// A drawing as the web client holds it: the same core the server runs, behind a small API that JavaScript
// calls through embind (see bindings.cpp). Everything goes in and out as text, and nothing throws. A call
// that can fail returns {"ok": true} or {"error": {"code", "message"}}, the server's error shape; the codes
// are the server's reject reasons, so the client can treat a local and a server rejection alike.
class Document {
public:
    // Applies an op, given as JSON as in core/tests/fixtures/ops. Codes: bad_op if it can't be decoded,
    // otherwise why it doesn't apply (shape_not_found, duplicate_id, ...). A failed op changes nothing.
    std::string applyOp(const std::string& opJson);

    // {"op": ...}: the op that undoes `opJson`, which must not have been applied yet; or an error, as for
    // applyOp.
    std::string invertOp(const std::string& opJson) const;

    // The ID of the topmost shape at (x, y), or "" if there's none.
    std::string hitTest(double x, double y) const;

    // The drawing as an SVG file.
    std::string toSVG() const;

    // Replaces the drawing with the shapes in an SVG file. Code invalid_svg, with the line and column in the
    // message, if it can't be parsed; the drawing is then unchanged.
    std::string loadSVG(const std::string& svg);

    // Replaces the drawing with a snapshot, in the server's format (see shapesJson). Code bad_snapshot if it
    // can't be used; the drawing is then unchanged.
    std::string loadJson(const std::string& snapshotJson);

    // {"version", "shapes": [{"id", "shape"}]}, bottom to top: everything needed to draw it. Text that isn't
    // valid UTF-8 can't be written as JSON; it's replaced with U+FFFD.
    std::string shapesJson() const;

private:
    Diagram diagram_;
};

}  // namespace vec::web

#endif  // VEC_WEB_DOCUMENT_H
