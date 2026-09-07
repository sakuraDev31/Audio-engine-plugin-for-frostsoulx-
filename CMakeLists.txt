cmake_minimum_required(VERSION 3.16)
project(frostsoulx_stereo_surround LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(frostsoulx_stereo_surround STATIC
    src/StereoSurroundProcessor.cpp
)
target_include_directories(frostsoulx_stereo_surround PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(frostsoulx_stereo_surround PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow
)

enable_testing()
add_executable(stereo_surround_tests
    tests/StereoSurroundProcessorTest.cpp
)
target_link_libraries(stereo_surround_tests PRIVATE frostsoulx_stereo_surround)
target_compile_options(stereo_surround_tests PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow
)
add_test(NAME stereo_surround_tests COMMAND stereo_surround_tests)
