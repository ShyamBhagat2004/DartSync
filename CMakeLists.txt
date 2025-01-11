cmake_minimum_required(VERSION 3.15)
project(DartSyncGUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# (Optional) Link statically on Windows
set(CMAKE_EXE_LINKER_FLAGS "-static")

# Enable wide-character (Unicode) APIs by default
add_definitions(-DUNICODE -D_UNICODE)

# Gather sources from src/
file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Build a WIN32 app
add_executable(DartSyncGUI WIN32 ${SOURCES})

# Link shell32 for SHBrowseForFolderW
target_link_libraries(DartSyncGUI PRIVATE shell32)
