cmake_minimum_required(VERSION 3.16)

project(SocketC LANGUAGES C)


#If you organize your directories structure, then you must declare in here, so the QT could find those source files correctly
add_executable(Client1 src/client/main.c
    src/unp.h
    include/config.h
    src/addrinfo.h
    src/error.c
    include/syslog.h)

add_executable(Client2 src/client/echo_client.c
    src/unp.h
    include/config.h
    src/addrinfo.h
    src/error.c
    include/syslog.h)

# This is for server
add_executable(ServerIter src/server/iterative-server.c
    src/unp.h
    include/config.h
    src/addrinfo.h
    src/error.c
    include/syslog.h
    src/server/wrapsock.c
    src/server/wrapunix.c
    src/server/wraplib.c)

add_executable(ServerConcur src/server/concurrent-server-unix.c
    src/unp.h
    include/config.h
    src/addrinfo.h
    src/error.c
    include/syslog.h
    src/server/wrapsock.c
    src/server/wrapunix.c
    src/server/wraplib.c)

include(GNUInstallDirs)
include_directories(${PROJECT_SOURCE_DIR}/include)

# Change this to Server or Client when build
install(TARGETS ServerIter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


if (WIN32)
    target_link_libraries(Client1 ws2_32)
    target_link_libraries(ServerIter ws2_32)
endif()
