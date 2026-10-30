add_library(vec_core
    src/core/GraphicsObject.cpp
    src/core/Circle.cpp
    src/core/Rectangle.cpp
    src/core/RoundedRectangle.cpp
    src/core/Line.cpp
    src/core/Hexagon.cpp
    src/core/FreehandSketch.cpp
    src/core/Text.cpp
    src/core/Diagram.cpp
    src/parser/SVGParser_Core.cpp
    src/parser/SVGParser_Parse.cpp
    src/ops/Apply.cpp
    src/ops/CommandStack.cpp
    src/ops/Json.cpp
)

target_include_directories(vec_core PUBLIC include)
target_compile_features(vec_core PUBLIC cxx_std_17)

find_package(tl-expected CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(vec_core PUBLIC tl::expected nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(vec_core PRIVATE /W4)
else()
    target_compile_options(vec_core PRIVATE -Wall -Wextra -pedantic)
endif()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()
