cmake_minimum_required(VERSION 3.10.2) 

project(sender)
project(receiver)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*.hpp")
add_executable(sender ${SOURCES})
add_executable(receiver ${SOURCES})

add_subdirectory(libs/CRCpp)
add_library(
    hashes
    libs/hashes/md5.h
    libs/hashes/md5.cpp
)

target_include_directories(sender PUBLIC "${CMAKE_SOURCE_DIR}/include/")
target_include_directories(receiver PUBLIC "${CMAKE_SOURCE_DIR}/include/")
target_link_libraries(sender PUBLIC CRCpp hashes)
target_link_libraries(receiver PUBLIC CRCpp hashes)

SET (CMAKE_CXX_FLAGS "-std=c++20 -g -DLINUX")

target_compile_definitions(sender PRIVATE SENDER)
target_compile_definitions(receiver PRIVATE RECEIVER)

