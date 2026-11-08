// Tests the built WebAssembly module from JavaScript, as the web client will use it. The C++ tests already
// cover Document's behaviour (core/tests/DocumentTest.cpp); these check what only the real module can show:
// that it loads, that embind passes values through, and that exceptions work inside WebAssembly.
//
//   VEC_WASM_DIR=build/wasm node --test core/wasm/vec.test.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import { before, test } from "node:test";
import { pathToFileURL } from "node:url";

const fixtures = resolve(import.meta.dirname, "../tests/fixtures");
const fixture = (name) => readFileSync(`${fixtures}/${name}`, "utf8");
const op = (name) => fixture(`ops/${name}.json`);

let vec;
before(async () => {
    const dir = process.env.VEC_WASM_DIR;
    assert.ok(dir, "set VEC_WASM_DIR to the folder with vec.js");
    const { default: createVecModule } = await import(pathToFileURL(resolve(dir, "vec.js")).href);
    vec = await createVecModule();
});

// A Document freed after the test, as embind objects must be.
function withDocument(body) {
    const doc = new vec.Document();
    try {
        body(doc);
    } finally {
        doc.delete();
    }
}

test("fixture ops apply, and their inverses undo them", () => withDocument((doc) => {
    assert.deepEqual(JSON.parse(doc.applyOp(op("add-rect"))), { ok: true });
    const before = doc.shapesJson();
    const inverse = JSON.parse(doc.invertOp(op("move"))).op;
    assert.deepEqual(JSON.parse(doc.applyOp(op("move"))), { ok: true });
    assert.notEqual(doc.shapesJson(), before);
    assert.deepEqual(JSON.parse(doc.applyOp(JSON.stringify(inverse))), { ok: true });
    assert.equal(doc.shapesJson(), before);
}));

test("bad input comes back as an error, which needs exceptions to work in WebAssembly", () => withDocument((doc) => {
    for (const input of ["", "{not json", "[]", '{"kind":"move"}']) {
        assert.equal(JSON.parse(doc.applyOp(input)).error.code, "bad_op", input);
    }
    assert.equal(JSON.parse(doc.loadJson("{not json")).error.code, "bad_snapshot");
}));

test("rejections use the server's reasons", () => withDocument((doc) => {
    doc.applyOp(op("add-rect"));
    assert.equal(JSON.parse(doc.applyOp(op("add-circle"))).error.code, "duplicate_id");
    assert.equal(JSON.parse(doc.applyOp('{"kind":"resize","id":"s_1","factor":1e308}')).error.code, "invalid_value");
    assert.equal(JSON.parse(doc.applyOp('{"kind":"deleteShape","id":"nope"}')).error.code, "shape_not_found");
}));

test("strings cross in both directions intact, including non-ASCII text", () => withDocument((doc) => {
    assert.deepEqual(JSON.parse(doc.applyOp(op("add-text"))), { ok: true });
    const text = JSON.parse(doc.shapesJson()).shapes[0].shape.text;
    assert.equal(text, JSON.parse(op("add-text")).shape.text);
    assert.equal(doc.hitTest(-1000, -1000), "");
}));

test("an SVG file loads, and a broken one says where", () => withDocument((doc) => {
    assert.deepEqual(JSON.parse(doc.loadSVG(fixture("app-export.svg"))), { ok: true });
    assert.equal(JSON.parse(doc.shapesJson()).shapes.length, 7);
    assert.match(JSON.parse(doc.loadSVG(fixture("malformed.svg"))).error.message, /^line 6, column \d+/);

    withDocument((copy) => {
        assert.deepEqual(JSON.parse(copy.loadJson(doc.shapesJson())), { ok: true });
        assert.equal(copy.toSVG(), doc.toSVG());
    });
}));

test("memory grows for large drawings", () => withDocument((doc) => {
    const shapes = Array.from({ length: 20000 }, (_, i) => `<rect x="${i}" y="1" width="2" height="3"/>`);
    assert.deepEqual(JSON.parse(doc.loadSVG(`<svg width="800" height="600">${shapes.join("")}</svg>`)), { ok: true });
    assert.equal(JSON.parse(doc.shapesJson()).shapes.length, 20000);
}));
