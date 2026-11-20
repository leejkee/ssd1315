cmake_minimum_required(VERSION 3.16)
project(ssd1315 VERSION 1.0.0 LANGUAGES C)

option(SSD1315_BUILD_EXAMPLE "Build ASCII example library" OFF)
option(SSD1315_BUILD_TESTS "Build host tests" OFF)

add_library(ssd1315 STATIC src/bsp_ssd1315.c)
add_library(ssd1315::ssd1315 ALIAS ssd1315)
target_include_directories(ssd1315 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(ssd1315 PUBLIC c_std_99)
set_target_properties(ssd1315 PROPERTIES C_EXTENSIONS OFF)

if(SSD1315_BUILD_EXAMPLE)
    add_library(ssd1315_ascii STATIC examples/ascii/ascii_example.c examples/ascii/font_en.c)
    add_library(ssd1315::ascii ALIAS ssd1315_ascii)
    target_include_directories(ssd1315_ascii PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/ascii"
    )
    target_link_libraries(ssd1315_ascii PUBLIC ssd1315::ssd1315)
endif()
if(SSD1315_BUILD_TESTS)
    enable_testing()
    add_executable(ssd1315_test tests/test_ssd1315.c)
    target_link_libraries(ssd1315_test PRIVATE ssd1315::ssd1315)
    add_test(NAME ssd1315_test COMMAND ssd1315_test)
endif()
