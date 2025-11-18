cmake_minimum_required(VERSION 3.5)

project(server_library_test_kit LANGUAGES CXX)

# --- Enable C standard ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)


#--- Env ---
set(C_FRAMEWORK "/home/gnar911/Desktop/20251106-SocketC")
set(CRAIE_LIB "/home/gnar911/Desktop/CraieLib_v1.8.3/CraieLib/build-CraieLib-Desktop-Debug")
set(ENV{C_FRAMEWORK} ${C_FRAMEWORK})

add_executable(craie_client main.cpp)


include_directories(
    ${C_FRAMEWORK}/framework
    /home/gnar911/Desktop/CraieLib_v1.8.3/CraieLib/include
    /home/gnar911/Desktop/CraieLib_v1.8.3/PluginVehicleInfo_250125/src
)

# --- Library directories for Debuging---
link_directories(
    ${CRAIE_LIB}/
)
target_link_libraries(craie_client
    /home/gnar911/Desktop/CraieLib_v1.8.3/CraieLib/build-CraieLib-Desktop-Debug/libCraieLib.so
    /home/gnar911/Desktop/CraieLib_v1.8.3/PluginVehicleInfo_250125/build-com.vehicleinfo-Desktop-Debug/libvehicelinfo.so
    Qt6::Core
    Qt6::Network
    #CraieLib
    )

set_target_properties(craie_client PROPERTIES
    BUILD_RPATH ${CRAIE_LIB}
    INSTALL_RPATH ${CRAIE_LIB}
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
