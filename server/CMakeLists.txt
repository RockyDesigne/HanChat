cmake_minimum_required(VERSION 3.25)
project(server)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

set(HEADERS
        include/channel.h
        include/blocking_iterator.h
        include/My_Exception.h
        include/Defer.h
        include/Socket.h
        include/channel.inl
        include/Message.h
        include/ServerSocket.h
        include/Winsocket.h
        include/ClientSocket.h
        include/ThreadPool.h include/DB.h)
set(SOURCES ./src/main.cpp src/ClientSocket.cpp src/ServerSocket.cpp src/Socket.cpp src/Winsocket.cpp src/Winsocket.cpp src/ThreadPool.cpp src/DB.cpp)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -Wextra -pedantic -ggdb -O3 -std=c++20
        )

# Set flags for static linking
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib odbc32.lib)
