cmake_minimum_required(VERSION 3.22)
project(khawasu_pc)

set(CMAKE_CXX_STANDARD 23)
set(FRESH_BUILD_SKIP_ESPIDF_COMPONENT 1)
set(FRESH_BUILD_SKIP_ESPIDF_STANDALONE 1)


if(NOT DEFINED ENV{KHAWASU_ROOT})
    message(FATAL_ERROR "KHAWASU_ROOT environment variable is not defined")
endif()

set(KHAWASU_ROOT $ENV{KHAWASU_ROOT})

add_subdirectory(${KHAWASU_ROOT}/thirdparties/fresh ${CMAKE_BINARY_DIR}/fresh)
add_subdirectory(${KHAWASU_ROOT}/khawasu/khawasu_core ${CMAKE_BINARY_DIR}/khawasu_core)

# link sockets
if (WIN32)
    message("WINDOWS detected")
    set(KHAWASU_PC_SRCS socket/winsocket.cpp)
    set(KHAWASU_PC_LIBS ws2_32 winmm)
endif ()
if (UNIX)
    message("UNIX detected")
    set(KHAWASU_PC_SRCS socket/unixsocket.cpp)
endif ()

set(KHAWASU_PC_SRCS ${KHAWASU_PC_SRCS} interfaces/mesh_socket_interface.cpp)
set(KHAWASU_PC_LIBS ${KHAWASU_PC_LIBS} fresh_static khawasu_core)

# Add PhyDeviceManager
set(KHAWASU_PC_SRCS ${KHAWASU_PC_SRCS} device_info.cpp phy_device_manager.cpp logical_callback_device.cpp)

add_executable(hwctrl main.cpp ${KHAWASU_PC_SRCS} arg_parser.h khawasu_app.cpp khawasu_app.h config.h)
add_executable(khawasu_pc_client main_client.cpp ${KHAWASU_PC_SRCS} arg_parser.h)
add_executable(khawasu_capi_test khawasu_capi_test.cpp c_api.cpp ${KHAWASU_PC_SRCS})

add_library(khawasu_pc STATIC ${KHAWASU_PC_SRCS})
add_library(khawasu_capi SHARED c_api.cpp ${KHAWASU_PC_SRCS})

target_include_directories(khawasu_pc PUBLIC "./" "socket/")

# Add TomlPlusPlus
target_include_directories(hwctrl PUBLIC thirdparties/tomlplusplus/)

target_link_libraries(hwctrl PUBLIC ${KHAWASU_PC_LIBS} comctl32)
target_link_libraries(khawasu_pc_client PUBLIC ${KHAWASU_PC_LIBS})
target_link_libraries(khawasu_pc PUBLIC ${KHAWASU_PC_LIBS})
target_link_libraries(khawasu_capi PUBLIC -static ${KHAWASU_PC_LIBS})
target_link_libraries(khawasu_capi_test PUBLIC ${KHAWASU_PC_LIBS})


target_compile_definitions(khawasu_pc PUBLIC KHAWASU_ENABLE_REFLECTION)
target_compile_definitions(hwctrl PUBLIC KHAWASU_ENABLE_REFLECTION)
target_compile_definitions(khawasu_capi PUBLIC KHAWASU_ENABLE_REFLECTION)
target_compile_definitions(khawasu_capi_test PUBLIC KHAWASU_ENABLE_REFLECTION)
target_compile_definitions(khawasu_pc_client PUBLIC KHAWASU_ENABLE_REFLECTION)

# Add Mess
#set(MESS_DIRECTORY "${KHAWASU_ROOT}\\thirdparties\\Mess")
#set(MESS_SOURCE "${KHAWASU_ROOT}\\khawasu\\khawasu_esp32_devices\\messages\\messages.ini")
#set(MESS_TARGET "${CMAKE_SOURCE_DIR}\\messages.cpp")
#
#include(${MESS_DIRECTORY}/CMakeLists.txt)
#
#add_dependencies(hwctrl mess)