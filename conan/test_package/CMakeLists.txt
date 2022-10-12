cmake_minimum_required(VERSION 3.23)
project(PackageTest CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}>)

find_package(Snacman CONFIG REQUIRED COMPONENTS snacman)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE ad::snacman)
