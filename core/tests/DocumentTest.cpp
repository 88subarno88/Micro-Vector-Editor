#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "wasm/Document.h"

using nlohmann::json;
using vec::web::Document;

namespace {

std::string fixture(const std::string& name) {
    std::ifstream in(std::string(FIXTURE_DIR) + "/" + name);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::string op(const std::string& name) { return fixture("ops/" + name + ".json"); }

std::string errorCode(const std::string& result) {
    const json j = json::parse(result);
    return j.contains("error") ? j["error"]["code"].get<std::string>() : "";
}

const std::string ok = R"({"ok":true})";

json shapes(const Document& doc) { return json::parse(doc.shapesJson())["shapes"]; }

}  // namespace

TEST(DocumentTest, EveryAddFixtureAddsItsShape) {
    for (const char* name : {"add-rect", "add-rounded-rect", "add-circle", "add-line", "add-hexagon",
                             "add-freehand", "add-text"}) {
        Document doc;
        EXPECT_EQ(doc.applyOp(op(name)), ok) << name;
        ASSERT_EQ(shapes(doc).size(), 1u) << name;
        EXPECT_EQ(shapes(doc)[0]["id"], json::parse(op(name))["id"]) << name;
    }
}

TEST(DocumentTest, EveryOtherFixtureAppliesToTheShapeItNames) {
    for (const char* name : {"move", "resize", "set-style-all", "set-style-one", "delete"}) {
        Document doc;
        ASSERT_EQ(doc.applyOp(op("add-rect")), ok);
        EXPECT_EQ(doc.applyOp(op(name)), ok) << name;
    }
}

TEST(DocumentTest, AFailedOpReturnsTheServersReasonAndChangesNothing) {
    Document doc;
    ASSERT_EQ(doc.applyOp(op("add-rect")), ok);
    const std::string before = doc.shapesJson();

    EXPECT_EQ(errorCode(doc.applyOp(R"({"kind":"move","id":"nope","dx":1,"dy":2})")), "shape_not_found");
    EXPECT_EQ(errorCode(doc.applyOp(op("add-rect"))), "duplicate_id");
    EXPECT_EQ(doc.shapesJson(), before);
}

TEST(DocumentTest, UndecodableOpsAreBadOps) {
    Document doc;
    for (const char* input :
         {"", "not json", "[]", "{}", R"({"kind":"move"})", R"({"kind":"explode","id":"a"})"}) {
        const json result = json::parse(doc.applyOp(input));
        EXPECT_EQ(result["error"]["code"], "bad_op") << input;
        EXPECT_FALSE(result["error"]["message"].get<std::string>().empty()) << input;
    }
}

TEST(DocumentTest, TheInverseOfAnOpUndoesIt) {
    for (const char* name : {"move", "resize", "set-style-all", "delete"}) {
        Document doc;
        ASSERT_EQ(doc.applyOp(op("add-rect")), ok);
        const std::string before = doc.shapesJson();

        const json inverse = json::parse(doc.invertOp(op(name)));
        ASSERT_TRUE(inverse.contains("op")) << name << ": " << inverse;
        ASSERT_EQ(doc.applyOp(op(name)), ok) << name;
        ASSERT_EQ(doc.applyOp(inverse["op"].dump()), ok) << name;

        EXPECT_EQ(doc.shapesJson(), before) << name;
    }
}

TEST(DocumentTest, InvertingAnOpThatCantApplyIsAnError) {
    Document doc;
    EXPECT_EQ(errorCode(doc.invertOp(op("move"))), "shape_not_found");
    EXPECT_EQ(errorCode(doc.invertOp("nonsense")), "bad_op");
}

TEST(DocumentTest, HitTestFindsTheTopmostShape) {
    Document doc;
    ASSERT_EQ(
        doc.applyOp(
            R"({"kind":"addShape","id":"below","shape":{"kind":"rect","x":0,"y":0,"width":100,)"
            R"("height":100,"cornerRadius":0,"style":{"stroke":"black","fill":"white","strokeWidth":1}}})"),
        ok);
    ASSERT_EQ(
        doc.applyOp(
            R"({"kind":"addShape","id":"above","shape":{"kind":"rect","x":50,"y":50,"width":100,)"
            R"("height":100,"cornerRadius":0,"style":{"stroke":"black","fill":"white","strokeWidth":1}}})"),
        ok);

    EXPECT_EQ(doc.hitTest(75, 75), "above");
    EXPECT_EQ(doc.hitTest(10, 10), "below");
    EXPECT_EQ(doc.hitTest(500, 500), "");
}

TEST(DocumentTest, LoadSvgReplacesTheDrawing) {
    Document doc;
    ASSERT_EQ(doc.applyOp(op("add-circle")), ok);

    EXPECT_EQ(doc.loadSVG(fixture("rect.svg")), ok);

    EXPECT_EQ(shapes(doc).size(), 2u);
    EXPECT_NE(doc.toSVG().find("<svg"), std::string::npos);
}

TEST(DocumentTest, AnSvgThatDoesNotParseSaysWhereAndChangesNothing) {
    Document doc;
    ASSERT_EQ(doc.applyOp(op("add-circle")), ok);
    const std::string before = doc.shapesJson();

    const json result = json::parse(doc.loadSVG(fixture("malformed.svg")));

    EXPECT_EQ(result["error"]["code"], "invalid_svg");
    EXPECT_NE(result["error"]["message"].get<std::string>().find("line 6"), std::string::npos) << result;
    EXPECT_EQ(doc.shapesJson(), before);
}

TEST(DocumentTest, ASnapshotFromShapesJsonLoadsTheSameDrawing) {
    Document original;
    ASSERT_EQ(original.loadSVG(fixture("app-export.svg")), ok);
    Document copy;

    EXPECT_EQ(copy.loadJson(original.shapesJson()), ok);

    EXPECT_EQ(copy.shapesJson(), original.shapesJson());
    EXPECT_EQ(copy.toSVG(), original.toSVG());
}

TEST(DocumentTest, ABadSnapshotIsRejectedAndChangesNothing) {
    Document doc;
    ASSERT_EQ(doc.applyOp(op("add-circle")), ok);
    const std::string before = doc.shapesJson();

    for (const char* input : {"", "{}", R"({"version":1,"shapes":"no"})"}) {
        EXPECT_EQ(errorCode(doc.loadJson(input)), "bad_snapshot") << input;
    }
    EXPECT_EQ(doc.shapesJson(), before);
}

TEST(DocumentTest, TextThatIsNotUtf8StillGivesJson) {
    Document doc;
    ASSERT_EQ(doc.loadSVG("<svg width=\"800\" height=\"600\"><text x=\"1\" y=\"2\">caf\xe9</text></svg>"),
              ok);

    const json drawing = json::parse(doc.shapesJson());

    EXPECT_EQ(drawing["shapes"][0]["shape"]["text"], "caf\xef\xbf\xbd");  // U+FFFD
}

TEST(DocumentTest, DrawCommandsFollowTheShapesBottomToTop) {
    Document doc;
    ASSERT_EQ(doc.loadSVG(fixture("app-export.svg")), ok);

    const json commands = json::parse(doc.drawCommands());
    const json shapes = json::parse(doc.shapesJson())["shapes"];

    ASSERT_EQ(commands.size(), shapes.size());  // every shape here draws with one call
    for (size_t i = 0; i < shapes.size(); ++i) EXPECT_EQ(commands[i]["id"], shapes[i]["id"]) << i;
}

TEST(DocumentTest, EachKindOfShapeDrawsWithItsPrimitive) {
    const auto drawn = [](const std::string& opName) {
        Document doc;
        EXPECT_EQ(doc.applyOp(op(opName)), ok) << opName;
        return json::parse(doc.drawCommands()).at(0);
    };

    const json rect = drawn("add-rounded-rect");
    EXPECT_EQ(rect["kind"], "rect");
    EXPECT_EQ(rect["x"], 10);
    EXPECT_EQ(rect["cornerRadius"], 5);
    EXPECT_EQ(rect["style"], (json{{"stroke", "red"}, {"fill", "#00ff00"}, {"strokeWidth", 2.5}}));

    const json circle = drawn("add-circle");
    EXPECT_EQ(circle["kind"], "ellipse");
    EXPECT_EQ(circle["cx"], 10);
    EXPECT_EQ(circle["rx"], 30);
    EXPECT_EQ(circle["ry"], 30);

    const json hexagon = drawn("add-hexagon");
    EXPECT_EQ(hexagon["kind"], "polyline");
    EXPECT_EQ(hexagon["closed"], true);
    EXPECT_EQ(hexagon["points"].size(), 12u);  // six corners

    const json freehand = drawn("add-freehand");
    EXPECT_EQ(freehand["kind"], "polyline");
    EXPECT_EQ(freehand["closed"], false);
    EXPECT_EQ(freehand["points"], (json{1, 2, 3.5, 4.5, 5, 6}));

    EXPECT_EQ(drawn("add-line")["kind"], "polyline");

    const json text = drawn("add-text");
    EXPECT_EQ(text["kind"], "text");
    EXPECT_EQ(text["text"], json::parse(op("add-text"))["shape"]["text"]);
    EXPECT_EQ(text["fontFamily"], "Arial");
}

TEST(DocumentTest, AnEmptyDrawingHasNoDrawCommands) { EXPECT_EQ(Document().drawCommands(), "[]"); }

TEST(DocumentTest, BoundsOfAShapeIsTheBoxAroundIt) {
    Document doc;
    ASSERT_EQ(doc.applyOp(op("add-rect")), ok);  // x 10, y 20, 30 by 40
    ASSERT_EQ(doc.applyOp(op("move")), ok);      // by 1.5, -2

    EXPECT_EQ(json::parse(doc.boundsOf("s_1")),
              (json{{"x", 11.5}, {"y", 18}, {"width", 30}, {"height", 40}}));

    Document circle;
    ASSERT_EQ(circle.applyOp(op("add-circle")), ok);  // centre (10, 20), radius 30
    EXPECT_EQ(json::parse(circle.boundsOf("s_1")),
              (json{{"x", -20}, {"y", -10}, {"width", 60}, {"height", 60}}));
}

TEST(DocumentTest, BoundsOfAnUnknownShapeIsNull) { EXPECT_EQ(Document().boundsOf("nope"), "null"); }

TEST(DocumentTest, ATranslatedShapeIsTheSameShapeMoved) {
    const Document doc;
    const auto moved = [&](const std::string& opName) {
        const json shape = json::parse(op(opName))["shape"];
        const json result = json::parse(doc.translatedShape(shape.dump(), 20, 20));
        EXPECT_TRUE(result.contains("shape")) << opName << ": " << result;
        return std::pair{shape, result["shape"]};
    };

    const auto [rect, movedRect] = moved("add-rect");
    EXPECT_EQ(movedRect["x"], rect["x"].get<double>() + 20);
    EXPECT_EQ(movedRect["y"], rect["y"].get<double>() + 20);
    EXPECT_EQ(movedRect["width"], rect["width"]);
    EXPECT_EQ(movedRect["style"], rect["style"]);

    const auto [circle, movedCircle] = moved("add-circle");
    EXPECT_EQ(movedCircle["center"], (json{{"x", 30}, {"y", 40}}));
    EXPECT_EQ(movedCircle["radius"], circle["radius"]);

    const auto [line, movedLine] = moved("add-line");
    EXPECT_EQ(movedLine["start"], (json{{"x", 21}, {"y", 22}}));
    EXPECT_EQ(movedLine["end"], (json{{"x", 23}, {"y", 24}}));

    const auto [sketch, movedSketch] = moved("add-freehand");
    EXPECT_EQ(movedSketch["points"],
              (json::array({{{"x", 21}, {"y", 22}}, {{"x", 23.5}, {"y", 24.5}}, {{"x", 25}, {"y", 26}}})));

    const auto [text, movedText] = moved("add-text");
    EXPECT_EQ(movedText["position"], (json{{"x", 30}, {"y", 40}}));
    EXPECT_EQ(movedText["text"], text["text"]);
}

TEST(DocumentTest, TranslatingABadShapeIsAnErrorAndLeavesTheDrawingAlone) {
    Document doc;
    ASSERT_EQ(doc.applyOp(op("add-rect")), ok);
    const std::string before = doc.shapesJson();

    EXPECT_EQ(errorCode(doc.translatedShape("not json", 1, 1)), "bad_shape");
    EXPECT_EQ(errorCode(doc.translatedShape(R"({"kind":"rect"})", 1, 1)), "bad_shape");
    EXPECT_EQ(errorCode(doc.translatedShape(json::parse(op("add-rect"))["shape"].dump(), 1e301, 0)),
              "bad_shape");
    EXPECT_EQ(doc.shapesJson(), before);
}
