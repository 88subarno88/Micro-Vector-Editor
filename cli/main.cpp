#include <lunasvg/lunasvg.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#include "ops/Json.h"
#include "parser/SVGParser.h"

namespace {

constexpr int exitOk = 0;
constexpr int exitFailed = 1;
constexpr int exitUsage = 2;

int usage() {
    std::cerr << "usage: vec validate <file.svg>\n"
                 "       vec convert <input> <output>   (.svg or .json, chosen by extension)\n"
                 "       vec export <input> --format png --out <output.png>\n";
    return exitUsage;
}

std::string extensionOf(const std::string& file) { return std::filesystem::path(file).extension().string(); }

// "file:line:column: message", or just the message if the error has no place in the file.
std::string describe(const std::string& file, const ParseError& error) {
    if (error.line == 0) return error.message;
    return file + ':' + std::to_string(error.line) + ':' + std::to_string(error.column) + ": " +
           error.message;
}

tl::expected<Diagram, std::string> load(const std::string& file) {
    const std::string extension = extensionOf(file);
    if (extension == ".svg") {
        auto diagram = SVGParser::loadSVG(file);
        if (!diagram) return tl::unexpected(describe(file, diagram.error()));
        return std::move(*diagram);
    }
    if (extension == ".json") {
        std::ifstream in(file);
        if (!in.is_open()) return tl::unexpected("cannot open " + file);

        const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
        auto diagram = vec::decodeDiagram(std::string_view(text));
        if (!diagram) return tl::unexpected(file + ": " + diagram.error().message);
        return std::move(*diagram);
    }
    return tl::unexpected(file + ": expected a .svg or .json file");
}

// The file's contents, built before opening it so that a failure leaves no partial output.
tl::expected<std::string, std::string> serialize(const Diagram& diagram, const std::string& file) {
    const std::string extension = extensionOf(file);
    if (extension == ".svg") return diagram.toSVG() + '\n';
    if (extension == ".json") {
        try {
            return vec::diagramToJson(diagram).dump(2) + '\n';
        } catch (const std::exception& e) {  // text that isn't valid UTF-8
            return tl::unexpected(file + ": " + e.what());
        }
    }
    return tl::unexpected(file + ": expected a .svg or .json file");
}

int validate(const std::string& file) {
    const auto diagram = SVGParser::loadSVG(file);
    if (!diagram) {
        std::cerr << describe(file, diagram.error()) << '\n';
        return exitFailed;
    }
    std::cout << file << ": ok, " << diagram->getObjectCount() << " shapes\n";
    return exitOk;
}

int convert(const std::string& input, const std::string& output) {
    const auto diagram = load(input);
    if (!diagram) {
        std::cerr << diagram.error() << '\n';
        return exitFailed;
    }
    const auto contents = serialize(*diagram, output);
    if (!contents) {
        std::cerr << contents.error() << '\n';
        return exitFailed;
    }

    std::ofstream out(output);
    out << *contents;
    out.close();
    if (!out) {
        std::cerr << "cannot write " << output << '\n';
        return exitFailed;
    }
    std::cout << output << ": wrote " << diagram->getObjectCount() << " shapes\n";
    return exitOk;
}

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

int exportPng(const std::string& input, const std::string& output) {
    const auto diagram = load(input);
    if (!diagram) {
        std::cerr << diagram.error() << '\n';
        return exitFailed;
    }

    if (!loadFallbackFont() && hasText(*diagram)) {
        std::cerr << "cannot draw text: no font found (install fonts-liberation or fonts-dejavu-core)\n";
        return exitFailed;
    }

    constexpr uint32_t opaqueWhite = 0xFFFFFFFF;
    const auto document = lunasvg::Document::loadFromData(diagram->toSVG());
    const lunasvg::Bitmap bitmap =
        document ? document->renderToBitmap(-1, -1, opaqueWhite) : lunasvg::Bitmap();
    if (bitmap.isNull() || !bitmap.writeToPng(output)) {
        std::cerr << "cannot write " << output << '\n';
        return exitFailed;
    }
    std::cout << output << ": wrote " << bitmap.width() << 'x' << bitmap.height() << " image\n";
    return exitOk;
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string_view command = argc > 1 ? argv[1] : "";
    if (argc == 3 && command == "validate") return validate(argv[2]);
    if (argc == 4 && command == "convert") return convert(argv[2], argv[3]);
    if (argc == 7 && command == "export" && std::string_view(argv[3]) == "--format" &&
        std::string_view(argv[4]) == "png" && std::string_view(argv[5]) == "--out") {
        return exportPng(argv[2], argv[6]);
    }
    return usage();
}
