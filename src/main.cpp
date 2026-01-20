#include <iostream>
#include <memory>
#include <fstream>
#include "core/GraphicsObject.h"
#include "core/Circle.h"
#include "core/Rectangle.h"
int main() {
    std::cout << "==========================================\n";
    std::cout << "    Testing Circle & rectangle Class\n";
    std::cout << "==========================================\n\n";
    
    // Creating a simple circle 
    std::cout << "Creating a circle at position (60, 60) with radius 40...\n";
    Circle myCircle(60, 60, 40);
    myCircle.setStrokeColor("blue");
    myCircle.setFillColor("yellow");
    myCircle.setStrokeWidth(2.0);
    
    std::cout << "Circle created with Center: (" << myCircle.getCenterX() 
              << ", " << myCircle.getCenterY() 
              << "), Radius: " << myCircle.getRadius() << "\n\n";

    // Creating a simple rectangle
    std::cout << "Creating a rectangle...\n";
    Rectangle rect(100, 100, 80, 50);
    rect.setStrokeColor("red");
    rect.setFillColor("pink");
    rect.setStrokeWidth(2.0);
    
    std::cout << "Rectangle created at -> (" << rect.getX() << ", " << rect.getY() 
              << ") with size " << rect.getWidth() << "x" << rect.getHeight() << "\n";
    
    // Converting in SVG format
    std::cout << "Converting to SVG format....\n";
    std::cout << myCircle.toSVG() << "\n\n";
    std::cout << rect.toSVG() << "\n\n";
    
    // Testing if points are inside or outside the circle
    std::cout << "Testing which points are inside the circle....\n";
    std::cout << "  Does (50, 50) inside the circle?  Ans-->" 
              << (myCircle.contains(50, 50) ? "Yes" : "No") << "\n";
    std::cout << "  Does (100, 100) inside the circle?   Ans-->" 
              << (myCircle.contains(100, 100) ? "Yes" : "No") << "\n";
    std::cout << "  Does  (70, 50) inside the circle?    Ans-->" 
              << (myCircle.contains(70, 50) ? "Yes" : "No") << " \n\n";
    
    // Testing if points are inside or outside the rectangle
    std::cout << "Testing which points are inside the rectangle....\n";
    std::cout << "  Does (140, 125) inside rectangle?  Ans-->" 
              << (rect.contains(140, 125) ? "Yes" : "No") << "\n";
    std::cout << " Does (50, 50) inside rectangle? Ans --> " 
              << (rect.contains(50, 50) ? "Yes" : "No") << "\n\n";
    
    
    //  Changing the circle's properties
    std::cout << "Changing circle position and size...\n";
    std::cout << "Before change--> " << myCircle.toSVG() << "\n";
    myCircle.setCenterX(100);
    myCircle.setCenterY(100);
    myCircle.setRadius(30);
    std::cout << "After change -->  " << myCircle.toSVG() << "\n\n";

    //Changing the rectangle's properties
    std::cout << "Changing rectangle position and size...\n";
    std::cout << "Before change-->" << rect.toSVG() << "\n";
    rect.setX(50);
    rect.setY(200);
    rect.setWidth(120);
    rect.setHeight(60);
    std::cout << "After change -->   " << rect.toSVG() << "\n\n";
    
    // Testing the circle clone functionality
    std::cout << "Making a copy of the circle...\n";
    auto circleCopy = myCircle.clone();
    std::cout << "Original circle--> " << myCircle.toSVG() << "\n";
    std::cout << "Copied circle-->  " << circleCopy->toSVG() << "\n\n";

    // Testing the rectangle clone functionality
    std::cout << "Making a copy of the rectangle...\n";
    auto rectCopy = rect.clone();
    std::cout << "Original rectangle --> " << rect.toSVG() << "\n";
    std::cout << "Copied rectangle -->  " << rectCopy->toSVG() << "\n\n";
    
    // Check if polymorphism works for circle
    std::cout << "Testing polymorphism for circle....\n";
    GraphicsObject* basePtr = &myCircle;
    std::cout << "Shape type -->" << basePtr->getType() << "\n";
    std::cout << "SVG output -->" << basePtr->toSVG() << "\n\n";

    //Check if polymorphism works for rectangle
    std::cout << "Testing polymorphism with rectangle ....\n";
    GraphicsObject* objPtr = &rect;
    std::cout << "Shape type --> " << objPtr->getType() << "\n";
    std::cout << "SVG output --> " << objPtr->toSVG() << "\n\n";
    
    // // Creating a few different circles to test variety
    // std::cout << "Creating several different circles:\n";
    // Circle firstCircle(25, 25, 20);
    // firstCircle.setStrokeColor("red");
    // firstCircle.setFillColor("pink");
    
    // Circle secondCircle(75, 75, 15);
    // secondCircle.setStrokeColor("green");
    // secondCircle.setFillColor("lightgreen");
    
    // std::cout << "Small -->" << firstCircle.toSVG() << "\n";
    // std::cout << "Medium -->" << secondCircle.toSVG() << "\n\n";
    
     // Creating an SVG file with both circles and rectangles
    std::cout << "Generating an SVG file with circles and rectangles...\n";
    std::ofstream file("test_shapes.svg");
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<svg width=\"400\" height=\"400\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    
    
    // Adding 2 test circles to the SVG file
    Circle c1(50, 50, 40);
    c1.setStrokeColor("blue");
    c1.setFillColor("yellow");
    c1.setStrokeWidth(2);
    file  << c1.toSVG() << "\n";
    
    Circle c2(150, 150, 30);
    c2.setStrokeColor("red");
    c2.setFillColor("pink");
    c2.setStrokeWidth(3);
    file << c2.toSVG() << "\n";


    //Adding 2 test rectangkles to the SVG file
    Rectangle r1(100, 100, 80, 60);
    r1.setStrokeColor("red");
    r1.setFillColor("pink");
    r1.setStrokeWidth(2);
    file  << r1.toSVG() << "\n";
    
    Rectangle r2(200, 50, 100, 40);
    r2.setStrokeColor("purple");
    r2.setFillColor("lavender");
    file  << r2.toSVG() << "\n";

    
    file << "</svg>\n";
    file.close();

    std::cout << "==========================================\n";
    std::cout << "All tests are done!\n";
    std::cout << "The Circle & Rectangle class is working .\n";
    std::cout << "==========================================\n\n";
    
    return 0;
}