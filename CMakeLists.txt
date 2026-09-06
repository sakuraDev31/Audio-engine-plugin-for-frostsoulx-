cmake_minimum_required(VERSION 3.20)
project(frostsoulx_audio_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(frostsoulx_dsp STATIC
    src/sound_field.cpp
    src/hrtf_binaural.cpp
    src/early_reflections.cpp
)

target_include_directories(frostsoulx_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(frostsoulx_dsp PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(frostsoulx_dsp PRIVATE /W4)
else()
    target_compile_options(frostsoulx_dsp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

enable_testing()
add_executable(frostsoulx_dsp_tests
    tests/test_sound_field.cpp
)
target_link_libraries(frostsoulx_dsp_tests PRIVATE frostsoulx_dsp)
add_test(NAME frostsoulx_dsp_tests COMMAND frostsoulx_dsp_tests)

option(FROSTSOULX_BUILD_EXAMPLE "Build a small command-line DSP example" ON)
if(FROSTSOULX_BUILD_EXAMPLE)
    add_executable(frostsoulx_dsp_example examples/basic_usage.cpp)
    target_link_libraries(frostsoulx_dsp_example PRIVATE frostsoulx_dsp)
endif()
