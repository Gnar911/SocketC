cmake_minimum_required(VERSION 3.10)
project(SocketTest C)

set(CMAKE_C_STANDARD 99)

add_executable(SocketTest main.c)

# Link the Windows socket library
if (WIN32)
    target_link_libraries(SocketTest ws2_32)
endif()

