cmake_minimum_required(VERSION 3.15)
project(DartSync)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Link statically on Windows (optional)
set(CMAKE_EXE_LINKER_FLAGS "-static")

# Include directories for project headers (if needed)
include_directories(${PROJECT_SOURCE_DIR})

# Collect all source files from the "src" folder or wherever your .cpp files are
# If your BackupManager.cpp, main.cpp, etc. are in a "src" folder:
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Define the executable target
add_executable(DartSync ${SOURCES})

# If you have other libraries to link in the future, add them here
# target_link_libraries(DartSync PRIVATE SomeOtherLibrary)
