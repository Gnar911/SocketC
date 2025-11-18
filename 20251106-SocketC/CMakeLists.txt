cmake_minimum_required(VERSION 3.16)

project(SocketC LANGUAGES C)
# So make sure to set the environment variables to the QT IDE build env to make it detect the env at compile time
# Do not set the env at the bash.rc since it only works at terminal start up
set(ROOT $ENV{ROOT})

#If you organize your directories structure, then you must declare in here, so the QT could find those source files correctly
add_executable(Client1 src/client/main.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h)

add_executable(Client2 src/client/echo_client.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
    src/wrapstdio.c)

add_executable(Client3 src/client/thread-client.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
    src/wrapstdio.c)

# This is for server
add_executable(ServerIter src/server/iterative-server.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
        src/wrapstdio.c)

add_executable(ServerV1 src/server/fork-server.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
        src/wrapstdio.c
                    src/signal.c)

add_executable(ServerV2 src/server/threads-server.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
    src/wrapstdio.c
    src/signal.c
    src/thread/wrappthread.c
    src/thread/unpthread.h)

add_executable(ServerV3 src/server/prethread_server_kernel_schedule.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
    src/wrapstdio.c
    src/signal.c
    src/thread/wrappthread.c
    src/thread/unpthread.h)

add_executable(ServerV4 src/server/prethread_server_user_schedule.c
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
    src/wrapstdio.c
    src/signal.c
    src/thread/wrappthread.c
    src/thread/unpthread.h)

add_executable(ServerV5 src/server/coroutine_tasks_server.cpp
    src/unp.h
    ${ROOT}/20251106-SocketC/framework/config.h
    src/addrinfo.h
    src/error.c
    ${ROOT}/20251106-SocketC/framework/fwlog.h
    src/wrapsock.c
    src/wrapunix.c
    src/wraplib.c
    src/wrapstdio.c
    src/signal.c
    src/thread/wrappthread.c
    src/thread/unpthread.h)

include(GNUInstallDirs)
include_directories(${PROJECT_SOURCE_DIR}/include
                    ${ROOT}/20251411-SocketC++/include/asio
                    ${ROOT}/20251411-SocketC++/include)

# Change this to Server or Client when build
install(TARGETS ServerV1
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


if (WIN32)
    target_link_libraries(Client1 ws2_32)
    target_link_libraries(ServerIter ws2_32)
endif()
