#include "core/Text.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>

/**
 * Text class 
 * ------- represents a string of text
 * ------- a text is defined by --->a)baseline point (x, y)
 *                              --->b) Content of the text
 *                              --->c) Font family 
 *                              --->d) Font size
 */
Text::Text(double x, double y, std::string content)
    : GraphicsObject(
        x,           // x (top-left of bounding box)
        y,           // y (top-left of bounding box)
        80,          //width (diameter)
        20           // height (diameter)
){
    text_ = content;          // Set the text content
    font_size_= 16;           // Default size is 16
    font_family_= "Arial";
    double width_guess = text_.length()*10;  // Rough guess
    setWidth(width_guess);
    setHeight(20);
}    
// Destructor
Text::~Text() {}
// Drawing the text using QPainter.
void Text::draw(QPainter* painter) {
    (void)painter;  // Suppress unused parameter warning
}
/**
 * Convert text to SVG format.
 */
std::string Text::toSVG() const {
    std::ostringstream svg;  
     //Start the text tag
    svg << "<text ";
    
    // Add position
    svg << "x=\"" << getX() << "\" ";
    svg << "y=\"" << getY() << "\" ";
    
    // Add font info
    svg << "font-size=\"" << font_size_ << "\" ";
    svg << "font-family=\"" << font_family_<< "\" ";
    
    // Add color
    svg << "fill=\"" << getStrokeColor() << "\"";
    
    // Close opening tag
    svg << ">";
    
    // Put the actual text
    svg <<text_;
    
    // Close the tag
    svg << "</text>";
    
    return svg.str();
    
}
//Create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> Text::clone() const {
    auto copy = std::make_unique<Text>(getX(), getY(),text_ );
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
    return copy;
}


//Get type,here "Text"
std::string Text::getType() const {
    return "Text";
}
//setters
void Text::setFrontSize(double size) {
    if (size <= 0) {
        throw std::invalid_argument("Font size must be positive.");
    }else{
        font_size_ = size;
    }
}
void Text::setText(const std::string& content) {
    text_ = content;
    double new_width= content.length()*10;  // Rough guess
    setWidth(new_width);

}