cmake_minimum_required(VERSION 3.0)
project(stm32-ecu)

string(FIND "${CMAKE_C_COMPILER}" "arm" ARM_COMPILER)

# Found that we're using an ARM compiler
if (NOT ARM_COMPILER EQUAL -1)
    set(BUILD_DIR "server")

    set(BUILD_SERVER ON)
    set(BUILD_CLIENT OFF)

# Not using an ARM compiler
else()
    set(BUILD_DIR "client")

    set(BUILD_SERVER OFF)
    set(BUILD_CLIENT ON)
endif()

add_subdirectory(CA7/stm32-ecu-manager ${BUILD_DIR})
