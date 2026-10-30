#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "ops/Apply.h"
#include "ops/Json.h"
#include "parser/SVGParser.h"

using namespace vec;
using nlohmann::json;

namespace {

const Style style{"red", "blue", 3};

// Checks `op` encodes to exactly `expected`, and that decoding `expected` encodes back the same.
void expectEncodes(const Operation& op, const char* expected) {
    const json encoded = op;
    EXPECT_EQ(encoded, json::parse(expected));
    EXPECT_EQ(json(json::parse(expected).get<Operation>()), encoded);
}

// Checks a shape survives JSON by comparing what it saves as, before and after.
void expectSameShapeAfterRoundTrip(const ShapeData& shape) {
    const Operation op = AddShape{"a", shape, std::nullopt};
    const Operation decoded = json(op).get<Operation>();

    Diagram original, restored;
    ASSERT_TRUE(apply(original, std::get<AddShape>(op)));
    ASSERT_TRUE(apply(restored, std::get<AddShape>(decoded)));
    EXPECT_EQ(restored.toSVG(), original.toSVG());
}

}  // namespace

TEST(JsonTest, DeleteShape) {
    expectEncodes(DeleteShape{"a"}, R"({"kind":"deleteShape","id":"a"})");
}

TEST(JsonTest, Move) {
    expectEncodes(Move{"a", 1.5, -2}, R"({"kind":"move","id":"a","dx":1.5,"dy":-2.0})");
}

TEST(JsonTest, Resize) {
    expectEncodes(Resize{"a", 2}, R"({"kind":"resize","id":"a","factor":2.0})");
}

TEST(JsonTest, SetStyleLeavesOutUnsetFields) {
    expectEncodes(SetStyle{"a", std::nullopt, "blue", std::nullopt},
                  R"({"kind":"setStyle","id":"a","fill":"blue"})");
    expectEncodes(SetStyle{"a", "red", std::nullopt, 2.0},
                  R"({"kind":"setStyle","id":"a","stroke":"red","strokeWidth":2.0})");
}

TEST(JsonTest, AddShapeWithAndWithoutAPosition) {
    const ShapeData shape{CircleGeom{{1, 2}, 3}, style};
    const char* const shapeJson =
        R"({"kind":"circle","center":{"x":1.0,"y":2.0},"radius":3.0,
            "style":{"stroke":"red","fill":"blue","strokeWidth":3.0}})";

    expectEncodes(AddShape{"a", shape, std::nullopt},
                  (R"({"kind":"addShape","id":"a","shape":)" + std::string(shapeJson) + "}").c_str());
    expectEncodes(AddShape{"a", shape, 4},
                  (R"({"kind":"addShape","id":"a","position":4,"shape":)" + std::string(shapeJson) + "}").c_str());
}

TEST(JsonTest, EveryKindOfShapeRoundTrips) {
    expectSameShapeAfterRoundTrip({RectGeom{1, 2, 3, 4}, style});
    expectSameShapeAfterRoundTrip({RectGeom{1, 2, 30, 40, 5}, style});
    expectSameShapeAfterRoundTrip({CircleGeom{{1, 2}, 3}, style});
    expectSameShapeAfterRoundTrip({LineGeom{{1, 2}, {3, 4}}, style});
    expectSameShapeAfterRoundTrip({HexagonGeom{{1, 2}, 3}, style});
    expectSameShapeAfterRoundTrip({FreehandGeom{{{1, 2}, {3, 4}}}, style});
    expectSameShapeAfterRoundTrip({TextData{{1, 2}, "a \"quoted\" <word>", 30}, style});
}

TEST(JsonTest, FreehandWithNoPointsRoundTrips) {
    const ShapeData shape{FreehandGeom{}, style};
    const json encoded = shape;

    EXPECT_TRUE(std::get<FreehandGeom>(encoded.get<ShapeData>().geometry).points.empty());
}

namespace {

const std::string validAdd =
    R"({"kind":"addShape","id":"a","position":0,"shape":{"kind":"freehand","points":[{"x":1,"y":2}],
        "style":{"stroke":"red","fill":"blue","strokeWidth":3}}})";

}  // namespace

TEST(DecodeTest, ValidTextDecodes) {
    const auto op = decodeOperation(std::string_view(R"({"kind":"move","id":"a","dx":1,"dy":2})"));

    ASSERT_TRUE(op) << op.error().message;
    EXPECT_DOUBLE_EQ(std::get<Move>(*op).dy, 2);
}

TEST(DecodeTest, ValidAddShapeDecodes) {
    EXPECT_TRUE(decodeOperation(std::string_view(validAdd)));
}

TEST(DecodeTest, FieldsOutsideTheFormatAreIgnored) {
    EXPECT_TRUE(decodeOperation(std::string_view(R"({"kind":"deleteShape","id":"a","extra":1})")));
}

TEST(DecodeTest, BadInputIsAnErrorWithAMessage) {
    const char* const badInputs[] = {
        "",
        "not json",
        "null",
        "[]",
        "42",
        R"({})",
        R"({"id":"a"})",                                         // no kind
        R"({"kind":"deleteShape"})",                             // no id
        R"({"kind":"explode","id":"a"})",                        // unknown kind
        R"({"kind":7,"id":"a"})",                                // kind isn't a string
        R"({"kind":"deleteShape","id":7})",                      // id isn't a string
        R"({"kind":"deleteShape","id":""})",                     // empty id
        R"({"kind":"move","id":"a","dx":1})",                    // no dy
        R"({"kind":"move","id":"a","dx":"1","dy":2})",           // number as a string
        R"({"kind":"move","id":"a","dx":true,"dy":2})",          // bool as a number
        R"({"kind":"move","id":"a","dx":null,"dy":2})",          // what NaN encodes to
        R"({"kind":"move","id":"a","dx":1e999,"dy":2})",         // too big for a double
        R"({"kind":"resize","id":"a"})",                         // no factor
        R"({"kind":"setStyle","id":"a","fill":7})",              // colour isn't a string
        R"({"kind":"setStyle","id":"a","strokeWidth":"wide"})",  // width isn't a number
        R"({"kind":"addShape","id":"a"})",                       // no shape
        R"({"kind":"addShape","id":"a","shape":{"kind":"blob"}})",  // unknown shape kind
        R"({"kind":"addShape","id":"a","shape":{"kind":"circle","center":{"x":1,"y":2},"radius":3}})",  // no style
        R"({"kind":"addShape","id":"a","position":-1,"shape":{}})",
        R"({"kind":"addShape","id":"a","position":1.5,"shape":{}})",
        R"({"kind":"addShape","id":"a","position":true,"shape":{}})",
        R"({"kind":"addShape","id":"a","shape":{"kind":"freehand","points":{"p":{"x":1,"y":2}},
            "style":{"stroke":"red","fill":"blue","strokeWidth":3}}})",  // points isn't an array
    };

    for (const char* input : badInputs) {
        const auto op = decodeOperation(std::string_view(input));
        EXPECT_FALSE(op) << input;
        if (!op) EXPECT_FALSE(op.error().message.empty()) << input;
    }
}

TEST(DecodeTest, EveryTruncationOfAValidMessageIsAnError) {
    for (size_t length = 0; length < validAdd.size(); ++length) {
        EXPECT_FALSE(decodeOperation(std::string_view(validAdd).substr(0, length))) << length;
    }
}

TEST(DecodeTest, TheParsedJsonOverloadReportsErrorsToo) {
    EXPECT_FALSE(decodeOperation(json{{"kind", "explode"}, {"id", "a"}}));
    EXPECT_TRUE(decodeOperation(json{{"kind", "deleteShape"}, {"id", "a"}}));
}

TEST(SnapshotTest, EmptyDiagramHasAnExactFormat) {
    EXPECT_EQ(diagramToJson(Diagram{}), json::parse(R"({"version":1,"shapes":[]})"));
}

TEST(SnapshotTest, OneShapeHasAnExactFormat) {
    Diagram diagram;
    ASSERT_TRUE(apply(diagram, AddShape{"a", {CircleGeom{{1, 2}, 3}, style}, std::nullopt}));

    EXPECT_EQ(diagramToJson(diagram), json::parse(R"({"version":1,"shapes":[{"id":"a","shape":
        {"kind":"circle","center":{"x":1.0,"y":2.0},"radius":3.0,
         "style":{"stroke":"red","fill":"blue","strokeWidth":3.0}}}]})"));
}

TEST(SnapshotTest, EveryFixtureRoundTripsThroughText) {
    for (const char* file : {"app-export.svg", "rect.svg", "circle.svg", "line.svg", "text.svg",
                             "polygon.svg", "path.svg"}) {
        const auto original = SVGParser::load_SVG(std::string(FIXTURE_DIR) + "/" + file);
        ASSERT_TRUE(original) << file;

        const std::string text = diagramToJson(*original).dump();
        const auto restored = decodeDiagram(std::string_view(text));

        ASSERT_TRUE(restored) << file << ": " << restored.error().message;
        EXPECT_EQ(restored->toSVG(), original->toSVG()) << file;
    }
}

TEST(SnapshotTest, BadSnapshotsAreErrors) {
    const char* const badInputs[] = {
        "",
        "null",
        "[]",
        R"({"shapes":[]})",                                  // no version
        R"({"version":2,"shapes":[]})",                      // another version
        R"({"version":"1","shapes":[]})",                    // version isn't a number
        R"({"version":1})",                                  // no shapes
        R"({"version":1,"shapes":{}})",                      // shapes isn't an array
        R"({"version":1,"shapes":[7]})",                     // entry isn't an object
        R"({"version":1,"shapes":[{"id":"a"}]})",            // no shape
        R"({"version":1,"shapes":[{"id":"","shape":{}}]})",  // empty id
    };

    for (const char* input : badInputs) {
        EXPECT_FALSE(decodeDiagram(std::string_view(input))) << input;
    }
}

TEST(SnapshotTest, ShapesThatApplyRejectsAreErrors) {
    const auto snapshot = [](const std::string& id, double fontSize) {
        const ShapeData text{TextData{{1, 2}, "Hi", 1}, style};
        json shape = text;
        shape["fontSize"] = fontSize;
        return json{{"id", id}, {"shape", shape}};
    };

    EXPECT_TRUE(decodeDiagram(json{{"version", 1}, {"shapes", {snapshot("a", 9), snapshot("b", 9)}}}));
    EXPECT_FALSE(decodeDiagram(json{{"version", 1}, {"shapes", {snapshot("a", 9), snapshot("a", 9)}}}));
    EXPECT_FALSE(decodeDiagram(json{{"version", 1}, {"shapes", {snapshot("a", 0)}}}));
    EXPECT_FALSE(decodeDiagram(json{{"version", 1}, {"shapes", {snapshot("a\"", 9)}}}));
}
