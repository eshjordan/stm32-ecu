cmake_minimum_required(VERSION 3.0)
project(stm32-ecu)

string(FIND "${CMAKE_C_COMPILER}" "arm" ARM_COMPILER)

# Found that we're using an ARM compiler
if (NOT ARM_COMPILER EQUAL -1)
    set(MANAGER_BUILD_DIR "stm32-ecu-manager-server")

    set(BUILD_SERVER ON)
    set(BUILD_CLIENT OFF)

# Not using an ARM compiler
else()
    set(MANAGER_BUILD_DIR "stm32-ecu-manager-client")

    set(BUILD_SERVER OFF)
    set(BUILD_CLIENT ON)
endif()

add_compile_definitions(STM32_BUILD)

add_subdirectory(CM4/ecu-dev)

add_subdirectory(CA7/stm32-ecu-manager ${CMAKE_CURRENT_BINARY_DIR}/${MANAGER_BUILD_DIR})

set(stm32_cube_ide /opt/st/stm32cubeide_1.9.0/headless-build.sh)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/workspace/
    COMMAND
        ${stm32_cube_ide} -data ${CMAKE_CURRENT_BINARY_DIR}/workspace/ -import ${CMAKE_CURRENT_SOURCE_DIR}/CM4/
)

add_custom_target(CM4 ALL
    ${stm32_cube_ide} -data ${CMAKE_CURRENT_BINARY_DIR}/workspace/ -build stm32-ecu_CM4/EngDebug
    BYPRODUCTS
        ${CMAKE_CURRENT_SOURCE_DIR}/CM4/EngDebug/stm32-ecu_CM4.elf
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/workspace/
)
