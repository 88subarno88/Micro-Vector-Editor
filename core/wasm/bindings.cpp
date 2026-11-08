// Exposes vec::web::Document to JavaScript. Built only with Emscripten, into vec.js and vec.wasm:
//
//   import createVecModule from "./vec.js";
//   const vec = await createVecModule();
//   const doc = new vec.Document();
//   doc.applyOp(JSON.stringify(op));
//   doc.delete();  // embind objects live in WebAssembly memory and must be freed
#include <emscripten/bind.h>

#include "Document.h"

EMSCRIPTEN_BINDINGS(vec) {
    using vec::web::Document;
    emscripten::class_<Document>("Document")
        .constructor<>()
        .function("applyOp", &Document::applyOp)
        .function("invertOp", &Document::invertOp)
        .function("hitTest", &Document::hitTest)
        .function("toSVG", &Document::toSVG)
        .function("loadSVG", &Document::loadSVG)
        .function("loadJson", &Document::loadJson)
        .function("shapesJson", &Document::shapesJson);
}
