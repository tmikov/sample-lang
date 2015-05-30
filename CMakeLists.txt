cmake_minimum_required(VERSION 3.2)
project(calc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES expr.cxx)
add_executable(calc ${SOURCE_FILES})