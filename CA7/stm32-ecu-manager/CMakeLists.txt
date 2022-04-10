
cmake_minimum_required(VERSION 3.0)

project(stm32-ecu-manager CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit")

add_compile_options(
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-exceptions"
    "-Wall"
    "-fstack-usage"
    "-g3"
    "-Og"
    "-MMD"
    "-MP"
    "-Wl,--specs-nosys.specs"
    "-Wl,-Map=stm32-ecu-manager.map"
    "-Wl,--gc-sections"
)

add_definitions("-DUSERSPACE_BUILD")

add_subdirectory(
    "shared"
)

include_directories(
    ${ECU_SHARED_INCLUDE}
    "../linux-5.10.61/build/install_artifact/include"
)

add_executable(stm32-ecu-manager
    main.cpp
    ${SHARED_SRC}
)

target_link_libraries(stm32-ecu-manager
    ecu_shared
)

add_custom_command(
    TARGET stm32-ecu-manager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
