cmake_minimum_required(VERSION 3.25)
project(client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

include_directories(include)

set(SOURCES ./src/main.cpp include/GUI.h)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++20)
target_include_directories(${PROJECT_NAME} PRIVATE "G:/projects/repos/libraries/raylib/include")
link_directories("G:/projects/repos/libraries/raylib/lib")

# Set flags for static linking
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32.lib
        "G:/projects/repos/libraries/raylib/lib/libraylib.a"
        winmm.lib gdi32.lib user32.lib shell32.lib)

