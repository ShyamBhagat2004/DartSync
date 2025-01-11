cmake_minimum_required(VERSION 3.15)
project(DartSync)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXE_LINKER_FLAGS "-static")
# Include directories for project headers
include_directories(src)

# Collect all source files in the src directory
file(GLOB SOURCES "src/*.cpp")

# Define the executable target
add_executable(DartSync ${SOURCES})

# If you have other libraries to link in the future, add them here
# Example:
# target_link_libraries(DartSync PRIVATE SomeOtherLibrary)
