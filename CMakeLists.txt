cmake_minimum_required(VERSION 3.15)
project(DartSyncGUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# (Optional) Link statically on Windows
set(CMAKE_EXE_LINKER_FLAGS "-static")

# -------------------------------
# Add these definitions to enable wide-character APIs:
add_definitions(-DUNICODE -D_UNICODE)
# -------------------------------

file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(DartSyncGUI WIN32 ${SOURCES})
