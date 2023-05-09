cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_language(CXX)

project(
    socket
    VERSION 0.1
    DESCRIPTION "Socket Programming Project"
    LANGUAGES CXX
)

add_executable(server src/server.cpp)
target_link_libraries(server stdc++)

add_executable(server.dbg src/server.cpp)
target_link_libraries(server.dbg stdc++)
target_compile_options(server.dbg PUBLIC -g)


add_executable(client src/client.cpp)
target_link_libraries(client stdc++)

add_executable(client.dbg src/client.cpp)
target_link_libraries(client.dbg stdc++)
target_compile_options(client.dbg PUBLIC -g)
