cmake_minimum_required(VERSION 3.26)
project(test_package LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(mr-manager REQUIRED)
find_package(GTest REQUIRED)

add_executable(test_package test.cpp)
target_link_libraries(test_package PRIVATE
    mr-manager::mr-manager
    GTest::gtest_main
)

enable_testing()
add_test(NAME test_package COMMAND test_package)
