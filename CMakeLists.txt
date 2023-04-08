cmake_minimum_required(VERSION 3.0)
project(stm32-ecu)


### Configuration

# One of EngDebug, EngRelease, ProdDebug, ProdRelease
if(NOT DEFINED CM4_CONFIG)
    set(CM4_CONFIG "ProdDebug")
endif()

option(BUILD_KERNEL "Build kernel modules for the Cortex-A7 chip. Must have cross-compilation env active." OFF)

set(STM32CUBEIDE_VERSION 1.11.2)

### Build settings

# Check if we're cross-compiling for ARM
string(FIND "${CMAKE_C_COMPILER}" "arm" ARM_COMPILER)
if(NOT ARM_COMPILER EQUAL -1)
    set(STM32_ECU_CROSS_COMPILING ON)
else()
    set(STM32_ECU_CROSS_COMPILING OFF)
endif()

if (STM32_ECU_CROSS_COMPILING)
    message(STATUS "ARM compiler detected, configuring build for cross-compilation")
    set(STM32_ECU_MANAGER_BINARY_DIR "stm32-ecu-manager-server")
    set(STM32_ECU_MANAGER_BUILD_SERVER ON)
    set(STM32_ECU_MANAGER_BUILD_CLIENT OFF)
    set(BUILD_SHARED_LIBS OFF)

else()
    message(STATUS "x86 compiler detected, configuring build for host compilation")
    set(STM32_ECU_MANAGER_BINARY_DIR "stm32-ecu-manager-client")
    set(STM32_ECU_MANAGER_BUILD_SERVER OFF)
    set(STM32_ECU_MANAGER_BUILD_CLIENT ON)
    set(BUILD_SHARED_LIBS ON)

endif()

# Set bin/lib output directories so we can access them easily
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_compile_definitions(STM32_BUILD)


### Build software

# Reusable ECU libraries
add_subdirectory(CM4/ecu-dev)


# Linux kernel modules, etc.
if(BUILD_KERNEL)
    if(NOT STM32_ECU_CROSS_COMPILING)
        message(FATAL_ERROR "BUILD_KERNEL is ON, but not cross compiling!")
    endif()
    add_subdirectory(CA7)
endif()


# ECU management client/server, runs in userspace, interfaces with custom kernel module
add_subdirectory(CA7/stm32-ecu-manager ${CMAKE_CURRENT_BINARY_DIR}/${STM32_ECU_MANAGER_BINARY_DIR})


# Set up STM32CubeIDE workspace before Cortex-M4 firmware build
set(STM32CUBEIDE /opt/st/stm32cubeide_${STM32CUBEIDE_VERSION}/headless-build.sh)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/workspace/
    COMMAND
        ${STM32CUBEIDE} -data ${CMAKE_CURRENT_BINARY_DIR}/workspace/ -import ${CMAKE_CURRENT_SOURCE_DIR}/CM4/
)


# Build Cortex-M4 firmware
add_custom_target(CM4_${CM4_CONFIG} ALL
    COMMAND
        ${STM32CUBEIDE} -data ${CMAKE_CURRENT_BINARY_DIR}/workspace/ -build stm32-ecu_CM4/${CM4_CONFIG}
    COMMAND
        cp ${CMAKE_CURRENT_SOURCE_DIR}/CM4/${CM4_CONFIG}/stm32-ecu_CM4.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    BYPRODUCTS
        ${CMAKE_CURRENT_SOURCE_DIR}/CM4/${CM4_CONFIG}/stm32-ecu_CM4.elf
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/workspace/
)


# Update CM4 firmware
install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/CM4/ProdDebug/stm32-ecu_CM4.elf
    DESTINATION
        ${CMAKE_BINARY_DIR}/install/home/root/
)
