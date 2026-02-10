#include "core/Text.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <QFont>
#include <QFontMetrics>

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
    double width_guess = text_.length()*20;  // Rough guess
    setWidth(width_guess);
    setHeight(40);
}    
// Destructor
Text::~Text() {}

// Drawing the text using QPainter.
void Text::draw(QPainter* painter) {
    if (!painter) return;

    // setting up pen 
    QPen pen{QColor(QString::fromStdString(getStrokeColor()))};
    painter->setPen(pen);

    //  setup fontsize and style
    QString family = QString::fromStdString(font_family_);
    // handling empty font family
    if (family.isEmpty()) family = "Arial"; 
    
    QFont font(family);
    font.setPointSizeF(font_size_);
    painter->setFont(font);

    // drawing text ; using bounding box (x,y,w,h) to align text
    QRectF rect(getX(), getY(), getWidth(), getHeight());
    
    // Qt::AlignLeft  and  Qt::AlignTop ensures  that text  starts exactly at x,y
    painter->drawText(rect, Qt::AlignLeft | Qt::AlignTop, QString::fromStdString(text_));
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

void Text::move(double dx, double dy) {
    GraphicsObject::move(dx, dy);
}

void Text::scale_factor(double factor) {
    font_size_ *= factor; 
    
    // Safety check
    if (font_size_ < 1) font_size_ = 1;

    // 2. Measure the NEW size of the text
    // (Use the same font family you use in draw())
    QFont font("Arial", static_cast<int>(font_size_)); 
    QFontMetrics metrics(font);
    
    // Calculate new dimensions based on the text string
    // (Assuming you have a variable 'textString' or 'content')
    QString qText = QString::fromStdString(text_); 
    
    double newWidth = metrics.horizontalAdvance(qText); 
    double newHeight = metrics.height();

    // 4. Update Object Dimensions
    // We add a small buffer (+5) to prevent the last letter from being clipped
    setWidth(newWidth +7); 
    setHeight(newHeight);
}