# The web client's view of the core. Built natively too, so its tests run with the others.
add_library(vec_web Document.cpp)
target_include_directories(vec_web PUBLIC ${PROJECT_SOURCE_DIR}/core)
target_link_libraries(vec_web PUBLIC vec_core)
if(MSVC)
    target_compile_options(vec_web PRIVATE /W4)
else()
    target_compile_options(vec_web PRIVATE -Wall -Wextra -pedantic)
endif()

# vec.js and vec.wasm: an ES module whose default export, createVecModule(), loads the core. The wasm
# preset builds with -fwasm-exceptions: decoding untrusted JSON catches nlohmann's exceptions, and without
# exception support a bad op would abort the module instead of returning bad_op.
if(EMSCRIPTEN)
    add_executable(vec_wasm bindings.cpp)
    target_link_libraries(vec_wasm PRIVATE vec_web)
    target_compile_options(vec_wasm PRIVATE -Wall -Wextra -pedantic)
    set_target_properties(vec_wasm PROPERTIES OUTPUT_NAME vec SUFFIX ".js" RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_link_options(vec_wasm PRIVATE
        -lembind
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sEXPORT_NAME=createVecModule
        -sALLOW_MEMORY_GROWTH=1
        -sENVIRONMENT=web,node
        --emit-tsd vec.d.ts)
    # A page that checks the module in a browser, with the op fixtures it applies. Copied whenever either
    # changes, not only when the module is relinked.
    file(GLOB opFixtures CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/core/tests/fixtures/ops/*.json)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/demo.html
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/demo.html ${CMAKE_BINARY_DIR}/demo.html
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/core/tests/fixtures/ops ${CMAKE_BINARY_DIR}/fixtures/ops
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/demo.html ${opFixtures})
    add_custom_target(vec_wasm_demo ALL DEPENDS ${CMAKE_BINARY_DIR}/demo.html)
endif()
