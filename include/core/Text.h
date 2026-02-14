#ifndef INCLUDE_TEXT_H
#define INCLUDE_TEXT_H

#include "core/GraphicsObject.h"
#include <cmath>
#include <string>

//Text class
//text is defined by baseline point (x, y),content of the text,font family and font size
class Text : public GraphicsObject {
private:
    std::string text_;  //the content
    std::string font_family_; //font family
    double font_size_;//font size

public:
    Text(double x, double y, std::string content);
    
    virtual ~Text();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    void scale_factor(double factor) override;
    
    //  setters for text
    void setFrontSize(double size);
    void setText(const std::string& content);
     // getters for text
    double getFontSize() const{ return font_size_;}
    std::string getText() const{ return text_; }
    
    
};

#endif // INCLUDE_TEXT_H