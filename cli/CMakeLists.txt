add_executable(vec main.cpp)
find_package(lunasvg CONFIG REQUIRED)
target_link_libraries(vec PRIVATE vec_core lunasvg::lunasvg)
set_target_properties(vec PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    target_compile_options(vec PRIVATE /W4)
else()
    target_compile_options(vec PRIVATE -Wall -Wextra -pedantic)
endif()

if(BUILD_TESTS)
    set(fixtures ${CMAKE_SOURCE_DIR}/core/tests/fixtures)

    # Adds a test that `vec` prints something matching `output`.
    function(add_vec_output_test name output)
        add_test(NAME vec_${name} COMMAND vec ${ARGN})
        set_tests_properties(vec_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${output}")
    endfunction()

    # Adds a test that `vec` exits with `code`.
    function(add_vec_exit_test name code)
        add_test(NAME vec_${name}_exit
                 COMMAND sh -c "\"\$0\" \"\$@\"; test \$? -eq ${code}" $<TARGET_FILE:vec> ${ARGN})
    endfunction()

    add_vec_output_test(validate_ok "app-export.svg: ok, 7 shapes" validate ${fixtures}/app-export.svg)
    add_vec_exit_test(validate_ok 0 validate ${fixtures}/app-export.svg)
    add_vec_output_test(validate_malformed "malformed.svg:6:[0-9]+: unclosed tag" validate ${fixtures}/malformed.svg)
    add_vec_exit_test(validate_malformed 1 validate ${fixtures}/malformed.svg)
    add_vec_output_test(validate_missing "cannot open" validate ${fixtures}/no-such-file.svg)
    add_vec_exit_test(validate_missing 1 validate ${fixtures}/no-such-file.svg)
    add_vec_output_test(no_args "usage: vec")
    add_vec_exit_test(no_args 2)
    add_vec_exit_test(unknown_command 2 explode file.svg)

    set(out ${CMAKE_CURRENT_BINARY_DIR}/convert)
    file(MAKE_DIRECTORY ${out})
    add_vec_output_test(convert_svg_to_json "wrote 7 shapes" convert ${fixtures}/app-export.svg ${out}/a.json)
    add_vec_exit_test(convert_to_unknown_type 1 convert ${fixtures}/rect.svg ${out}/a.png)
    add_vec_exit_test(convert_malformed 1 convert ${fixtures}/malformed.svg ${out}/bad.json)
    add_vec_exit_test(convert_bad_json 1 convert ${fixtures}/ops/move.json ${out}/bad.svg)
    add_vec_exit_test(convert_missing_input 1 convert ${out}/no-such-file.json ${out}/bad.svg)
    add_vec_exit_test(convert_to_missing_directory 1 convert ${fixtures}/rect.svg ${out}/no-such-dir/a.json)
    add_vec_exit_test(convert_missing_argument 2 convert ${fixtures}/rect.svg)

    add_vec_output_test(export_png "wrote 800x600 image" export ${fixtures}/rect.svg --format png --out ${out}/rect.png)
    add_vec_exit_test(export_png 0 export ${fixtures}/rect.svg --format png --out ${out}/rect.png)
    add_vec_exit_test(export_malformed 1 export ${fixtures}/malformed.svg --format png --out ${out}/bad.png)
    add_vec_exit_test(export_to_missing_directory 1 export ${fixtures}/rect.svg --format png --out ${out}/no-such-dir/a.png)
    add_vec_exit_test(export_unknown_format 2 export ${fixtures}/rect.svg --format gif --out ${out}/a.gif)
    add_vec_exit_test(export_missing_out 2 export ${fixtures}/rect.svg --format png)

    # svg -> json -> svg -> json must settle: both json files match.
    add_test(NAME vec_convert_round_trip
             COMMAND sh -c "set -e; vec=\$0; dir=\$1
                 \"\$vec\" convert \"\$2\" \"\$dir/1.json\"
                 \"\$vec\" convert \"\$dir/1.json\" \"\$dir/2.svg\"
                 \"\$vec\" convert \"\$dir/2.svg\" \"\$dir/3.json\"
                 cmp \"\$dir/1.json\" \"\$dir/3.json\""
                 $<TARGET_FILE:vec> ${out} ${fixtures}/app-export.svg)
endif()
