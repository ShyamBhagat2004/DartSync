cmake_minimum_required(VERSION 3.15)
project(DartSyncGUI)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# (Optional) Link statically on Windows
set(CMAKE_EXE_LINKER_FLAGS "-static")

# Enable wide-character (Unicode) APIs by default
add_definitions(-DUNICODE -D_UNICODE)

# Gather sources/headers from src/
file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Build a WIN32 app (so we get WinMain, not a console)
add_executable(DartSyncGUI WIN32 ${SOURCES})

# Link shell32 for SHBrowseForFolder
# (Depending on your environment, this might be optional)
target_link_libraries(DartSyncGUI PRIVATE shell32)
