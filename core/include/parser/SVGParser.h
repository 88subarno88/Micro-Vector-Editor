#ifndef SVGPARSER_H
#define SVGPARSER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include "core/Diagram.h"

class SVGParser {
public:
    using Attributes = std::unordered_map<std::string, std::string>;

    //read files and load into diagram
    static void load_SVG(const std::string& filename, Diagram& diagram);

    // Parses the name="value" pairs of one opening tag, e.g. <rect x="1" rx='2'/>.
    // Stops at the closing '>'; on malformed input returns the pairs read so far.
    static Attributes parseAttributes(std::string_view tag);

    // Parses the leading number of `text` ("10", " +1.5e2", "10px").
    // Returns `fallback` for empty, non-numeric or non-finite input. Never throws.
    static double parseNumber(std::string_view text, double fallback);

    // Returns the next tag in `doc` at or after `pos` ("<rect .../>", "</svg>") and moves `pos`
    // past it. Skips comments, <?...?> and <!...>. Returns "" at the end or on an unclosed tag.
    static std::string_view nextTag(std::string_view doc, size_t& pos);

private:
    // Adds the shape described by one tag (if any); `content` is the text following the tag.
    static void parse_Tag(std::string_view tag, std::string_view content, Diagram& diagram);
};

#endif