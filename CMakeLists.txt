# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B Build -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B Build -S .

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C or C++)
project(C_Allocator VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# The library: everything in src/ (the Arduino library layout)
file(GLOB C_ALLOCATOR_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c" "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.h")
add_library(C_AllocatorLib STATIC ${C_ALLOCATOR_SOURCES})
target_include_directories(C_AllocatorLib PUBLIC "${CMAKE_SOURCE_DIR}/src")

# Desktop example allocators (examples/desktop)
add_subdirectory(examples/desktop)

# Collect all .cpp and *.h source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")

# Add an executable target
add_executable(C_AllocatorApp ${SOURCES})

target_link_libraries(C_AllocatorApp PRIVATE
    C_AllocatorLib
    ExamplesLib
)
