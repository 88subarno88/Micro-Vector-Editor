#ifndef SVGPARSER_H
#define SVGPARSER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <tl/expected.hpp>
#include "core/Diagram.h"

// Why loading failed. line and column are 1-based; both are 0 when the error is not
// at a place in the document (e.g. the file could not be opened).
struct ParseError {
    int line = 0;
    int column = 0;
    std::string message;
};

class SVGParser {
public:
    using Attributes = std::unordered_map<std::string, std::string>;

    // Reads `filename` and parses it with parse_SVG.
    static tl::expected<Diagram, ParseError> load_SVG(const std::string& filename);

    // Builds a diagram from an SVG document. Unknown tags are skipped; an unclosed tag or
    // comment is an error.
    static tl::expected<Diagram, ParseError> parse_SVG(std::string_view doc);

    // Parses the name="value" pairs of one opening tag, e.g. <rect x="1" rx='2'/>.
    // Stops at the closing '>'; on malformed input returns the pairs read so far.
    static Attributes parseAttributes(std::string_view tag);

    // Parses the leading number of `text` ("10", " +1.5e2", "10px").
    // Returns `fallback` for empty, non-numeric or non-finite input. Never throws.
    static double parseNumber(std::string_view text, double fallback);

    // Returns the next tag in `doc` at or after `pos` ("<rect .../>", "</svg>") and moves `pos`
    // past it. Skips comments, <?...?> and <!...>. Returns "" at the end (pos = doc.size())
    // or on an unclosed tag or comment (pos = its '<').
    static std::string_view nextTag(std::string_view doc, size_t& pos);

    // Replaces &lt; &gt; &amp; &quot; &apos; with their characters; anything else is kept as is.
    static std::string decodeEntities(std::string_view text);

private:
    // Adds the shape described by one tag (if any); `content` is the text following the tag.
    static void parse_Tag(std::string_view tag, std::string_view content, Diagram& diagram);
};

#endif
