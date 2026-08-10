cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
set (CMAKE_CXX_STANDARD 11)
project(PackageTest CXX)

# Include the Conan-generated files
find_package(xmsgridtrace CONFIG REQUIRED)

add_executable(example example.cpp)

# Link Conan dependencies
target_link_libraries(example)
