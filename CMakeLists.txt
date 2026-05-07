cmake_minimum_required(VERSION 3.14)
project(LogTriagePP CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent downloads dependencies automatically — like pip install, but for C++
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Sources shared between the main binary and the test binary
set(LT_SOURCES
    src/model.cpp
    src/parser.cpp
    src/cluster.cpp
    src/output.cpp
    src/io.cpp
)

# Main executable: logtriage
add_executable(logtriage ${LT_SOURCES} src/main.cpp)
target_include_directories(logtriage PRIVATE src)

# Test binary: logtriage_tests
add_executable(logtriage_tests ${LT_SOURCES} tests/test_core.cpp)
target_include_directories(logtriage_tests PRIVATE src)
target_link_libraries(logtriage_tests gtest_main)

include(GoogleTest)
gtest_discover_tests(logtriage_tests)
