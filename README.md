# Micro-Vector Editor

[![CI](https://github.com/88subarno88/Micro-Vector-Editor/actions/workflows/ci.yml/badge.svg)](https://github.com/88subarno88/Micro-Vector-Editor/actions/workflows/ci.yml)

A small vector graphics editor in C++17 with a Qt 6 desktop app. Its core, which holds the shapes, the SVG parser and the edit history, has no Qt dependency, so the same code can later run on a server and in the browser.

**Status:** the desktop editor and core work today. A collaborative C++ backend and a web client are planned but not built yet.

## Features

- **Shapes:** rectangle, rounded rectangle, circle, line, hexagon, freehand sketch and text
- **Editing:** draw, drag to move, and right-click to set fill, stroke, stroke width or size, or to delete
- **Keyboard:** undo (`Ctrl+Z`), redo (`Ctrl+Y`), copy, cut, paste and delete
- **Files:** new, open, save and save as, using SVG. Shape IDs are kept across saves
- **SVG parser:** hand-written, with no XML library. A malformed file reports the line and column of the error

## How it works

```text
desktop/  Qt app: window, toolbar, canvas, QtRenderer
   │ builds operations, draws through the Renderer interface
   ▼
core/     no Qt
   ├── core/    shapes (GraphicsObject), Diagram, Renderer interface
   ├── ops/     Operation, apply, invert, CommandStack, JSON
   └── parser/  SVG parser
```

- **Every edit is an operation.** An `Operation` is a `std::variant` of plain data: `AddShape`, `DeleteShape`, `Move`, `Resize` or `SetStyle`.
- **`apply` returns errors instead of crashing.** Bad input, such as an unknown ID, NaN or a number too large to save, gives an `ApplyError` and leaves the diagram unchanged.
- **Undo stores operations, not shapes.** `invert` returns the operation that undoes another one, and `CommandStack` keeps each pair. Shapes are referenced by a stable `ShapeId`, never by a pointer.
- **Operations and diagrams convert to JSON.** The decoders treat input as untrusted and return an error on anything malformed.
- **Shapes don't know about Qt.** They draw through a small `Renderer` interface, and `QtRenderer` implements it in the desktop app.

## Build and run

Tested on Ubuntu 24.04 with GCC 13 and Qt 6.4. Dependencies other than Qt come from [vcpkg](https://github.com/microsoft/vcpkg).

```bash
sudo apt install build-essential cmake git curl zip unzip tar pkg-config qt6-base-dev libgl-dev

git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=~/vcpkg

cmake --preset dev
cmake --build --preset dev --parallel
./build/dev/micro-vector-editor
```

To build only the core and tests, without Qt, add `-DBUILD_DESKTOP=OFF` to the `cmake --preset dev` line.

## Command-line tool

The build also produces `build/dev/vec`, which doesn't need Qt:

```bash
vec validate drawing.svg            # exit code 1 and file:line:column on a parse error
vec convert drawing.svg drawing.json
vec convert drawing.json drawing.svg
vec export drawing.json --format png --out drawing.png
```

## WebAssembly

The same core also compiles to WebAssembly for the browser, as `build/wasm/vec.js` and `vec.wasm`. It needs [Emscripten](https://emscripten.org/docs/getting_started/downloads.html):

```bash
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
~/emsdk/emsdk install 6.0.13 && ~/emsdk/emsdk activate 6.0.13
source ~/emsdk/emsdk_env.sh                # in each new terminal

cmake --preset wasm
cmake --build --preset wasm --parallel
python3 -m http.server -d build/wasm 8000  # then open http://localhost:8000/demo.html
VEC_WASM_DIR=build/wasm node --test core/wasm/vec.test.mjs   # tests the built module, as CI does
```

The demo page loads the module, applies a few operations and shows the resulting SVG. From JavaScript:

```js
import createVecModule from "./vec.js";
const vec = await createVecModule();
const doc = new vec.Document();
doc.applyOp(JSON.stringify(op));  // {"ok":true} or {"error":{"code","message"}}
doc.delete();                     // objects from WebAssembly must be freed
```

## Testing

```bash
ctest --preset dev                  # unit tests (GoogleTest)

cmake --preset asan                 # the same tests under ASan and UBSan
cmake --build --preset asan --parallel
ctest --preset asan
```

The server's tests also run under ThreadSanitizer, with every dependency rebuilt under it too, so the first configure takes a while. TSan can't start with the address randomisation newer kernels use, so run it under `setarch -R`:

```bash
setarch -R cmake --preset tsan
setarch -R cmake --build --preset tsan --parallel
docker compose -f deploy/docker-compose.dev.yml up -d --wait
set -a; source deploy/dev.env; set +a
dbmate -e TEST_DATABASE_URL up
setarch -R ctest --preset tsan
```

Reports that aren't ours to fix are suppressed, with the reason for each, in `server/tests/tsan.supp`.

The fuzz targets cover the SVG parser and the JSON decoders. They need clang:

```bash
cmake --preset fuzz
cmake --build --preset fuzz --parallel
mkdir -p build/fuzz/corpus_parser build/fuzz/corpus_op

build/fuzz/core/fuzz/fuzz_parser -max_total_time=600 -dict=core/fuzz/svg.dict \
  -artifact_prefix=build/fuzz/ build/fuzz/corpus_parser core/tests/fixtures
build/fuzz/core/fuzz/fuzz_op_json -max_total_time=600 -dict=core/fuzz/op_json.dict \
  -artifact_prefix=build/fuzz/ build/fuzz/corpus_op core/tests/fixtures/ops
```

Each push runs CI on GitHub Actions: the full build, a core-only build with clang-tidy, the tests under sanitizers, a format check, and 60 seconds of each fuzzer.

## Known limitations

The parser reads a subset of SVG, aimed at files this editor saves. Importing a file from another tool can lose detail:

- **Polygons:** any `<polygon>` becomes a regular hexagon that fits its width. Its original shape is lost.
- **Paths:** only `M` and `L` commands are read, so curves are not. Paths also load unfilled.
- **Other SVG features:** groups, transforms, gradients, CSS styles and `viewBox` are ignored. Saved files always use an 800×600 canvas.
- **Text:** the font family isn't imported, and the text box size is an estimate. Exported PNGs always use Liberation Sans or DejaVu Sans, and text is placed differently from the editor: an SVG `y` is the text baseline, while the editor treats it as the top of the text.
- **Very large numbers:** shapes with coordinates beyond ±1e300 are skipped on load.

In the editor:

- **Clicking:** hexagons are hit-tested as circles, rounded rectangles as plain rectangles, and freehand sketches by their bounding box.
- **Selection:** there's no visible highlight on the selected shape.
- **Resize:** resizing uses a percentage dialog. There are no drag handles.
- **History:** opening a file or starting a new one clears undo history.
