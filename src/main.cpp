#include <iostream>
#include "core/GraphicsObject.h"

class TestShape : public GraphicsObject {
public:
    using GraphicsObject::GraphicsObject; 
    void draw(QPainter* painter) override {}
    std::string toSVG() const override { 
        return "<test-shape />"; 
    }
    std::unique_ptr<GraphicsObject> clone() const override { 
        return nullptr; 
    }
    std::string getType() const override { 
        return "TestShape"; 
    }
};

// Main Test Function
int main() {
    std::cout << "---> <--- Starting GraphicsObject Test ---><--- \n";
    //Create a Test Object (at x=10, y=20, w=100, h=50)
    TestShape obj(10, 20, 100, 50);
    //Test Getters
    std::cout << "Initial Position: (" << obj.getX() << ", " << obj.getY() << ")\n";
    // Test Setters
    obj.setX(99);
    obj.setFillColor("red");
    std::cout << "New X Position: " << obj.getX() << " (Expected: 99)\n";
    std::cout << "Fill Color: " << obj.getFillColor() << " (Expected: red)\n";
    // Test Contains Method 
    bool hit = obj.contains(110, 30); // (Hit detection)---> Should be inside btw (99+100 width covers this)
    std::cout << "Hit Test (110,30): " << (hit ? "YES" : "NO") << "\n";
    std::cout << "---><--- ✅Test Finished Successfully! ---><---\n";
    return 0;
}