cmake_minimum_required(VERSION 3.22)
project(khawasu_pc)

set(CMAKE_CXX_STANDARD 23)
set(FRESH_BUILD_SKIP_ESPIDF_COMPONENT 1)
set(FRESH_BUILD_SKIP_ESPIDF_STANDALONE 1)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})


if(NOT DEFINED ENV{KHAWASU_ROOT})
    message(FATAL_ERROR "KHAWASU_ROOT environment variable is not defined")
endif()

set(KHAWASU_ROOT $ENV{KHAWASU_ROOT})

add_subdirectory(${KHAWASU_ROOT}/thirdparties/fresh ${CMAKE_BINARY_DIR}/fresh)
add_subdirectory(${KHAWASU_ROOT}/khawasu/khawasu_core ${CMAKE_BINARY_DIR}/khawasu_core)

# link sockets
if (WIN32)
    set(KHAWASU_PC_SRCS socket/winsocket.cpp)
    set(KHAWASU_PC_LIBS ws2_32 winmm)
endif ()
if (UNIX)
    set(KHAWASU_PC_SRCS socket/unixsocket.cpp)
endif ()

set(KHAWASU_PC_SRCS ${KHAWASU_PC_SRCS} interfaces/mesh_socket_interface.cpp)
set(KHAWASU_PC_LIBS ${KHAWASU_PC_LIBS} khawasu_core)

# Add PhyDeviceManager
set(KHAWASU_PC_SRCS ${KHAWASU_PC_SRCS} device_info.cpp phy_device_manager.cpp logical_callback_device.cpp)

add_executable(khawasu_pc_server main.cpp ${KHAWASU_PC_SRCS})
add_executable(khawasu_pc_client main_client.cpp ${KHAWASU_PC_SRCS})

add_library(khawasu_pc SHARED ${KHAWASU_PC_SRCS})
add_library(khawasu_capi SHARED c_api.cpp ${KHAWASU_PC_SRCS})

target_include_directories(khawasu_pc PUBLIC "./" "socket/")

target_link_libraries(khawasu_pc_server PUBLIC ${KHAWASU_PC_LIBS})
target_link_libraries(khawasu_pc_client PUBLIC ${KHAWASU_PC_LIBS})
target_link_libraries(khawasu_pc PUBLIC ${KHAWASU_PC_LIBS})
target_link_libraries(khawasu_capi PUBLIC -static ${KHAWASU_PC_LIBS})

add_compile_options(-fdiagnostics-color=always)