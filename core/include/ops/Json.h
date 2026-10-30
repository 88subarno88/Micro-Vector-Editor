#ifndef OPS_JSON_H
#define OPS_JSON_H

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

#include "core/Diagram.h"
#include "ops/Operation.h"

namespace vec {

// Each object names what it is in a "kind" field, e.g. {"kind":"move","id":"s_1","dx":1,"dy":2}.
// Optional fields are left out when unset. from_json throws on anything it can't decode;
// use decodeOperation for input from outside the program.
void to_json(nlohmann::json& j, const ShapeData& shape);
void from_json(const nlohmann::json& j, ShapeData& shape);

void to_json(nlohmann::json& j, const Operation& op);
void from_json(const nlohmann::json& j, Operation& op);

// Why an operation couldn't be decoded.
struct DecodeError {
    std::string message;
};

// Decode untrusted input. An unknown kind, a missing field or a wrong type is an error;
// no exception escapes. Fields that aren't part of the format are ignored.
tl::expected<Operation, DecodeError> decodeOperation(const nlohmann::json& j);
tl::expected<Operation, DecodeError> decodeOperation(std::string_view text);

// A whole diagram as a snapshot: {"version":1,"shapes":[{"id":"s_1","shape":{...}}, ...]},
// with shapes in drawing order. Dumping it throws if a shape's text isn't valid UTF-8.
nlohmann::json diagramToJson(const Diagram& diagram);

// Decode untrusted input, under the same rules as decodeOperation. A snapshot from another
// version, a repeated id or a shape that apply() would reject is an error too.
tl::expected<Diagram, DecodeError> decodeDiagram(const nlohmann::json& j);
tl::expected<Diagram, DecodeError> decodeDiagram(std::string_view text);

}  // namespace vec

#endif // OPS_JSON_H
