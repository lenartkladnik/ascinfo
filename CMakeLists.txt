cmake_minimum_required(VERSION 3.20)

project(
    ascinfo
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_executable(ascinfo ascinfo.cpp)
