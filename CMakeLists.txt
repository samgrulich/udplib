cmake_minimum_required(VERSION 3.10.2) 

project(sender)
project(receiver)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*.hpp")

add_executable(sender ${SOURCES})
add_executable(receiver ${SOURCES})

SET (CMAKE_CXX_FLAGS "-std=c++20 -g -DLINUX")

target_compile_definitions(sender PRIVATE SENDER)
target_compile_definitions(receiver PRIVATE RECEIVER)

