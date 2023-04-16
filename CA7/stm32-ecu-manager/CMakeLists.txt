cmake_minimum_required(VERSION 3.5.1)
cmake_policy(SET CMP0077 NEW)

# Project
project(stm32-ecu-manager C CXX)

option(STM32_ECU_MANAGER_BUILD_SERVER OFF)
option(STM32_ECU_MANAGER_BUILD_CLIENT ON)

if(STM32_ECU_MANAGER_BUILD_SERVER AND STM32_ECU_MANAGER_BUILD_CLIENT)
  message(FATAL_ERROR "Can't build both stm32-ecu-manager server and client simultaneously")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=gnu++14 -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit")

add_compile_options(
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-exceptions"
    "-Wall"
    "-fstack-usage"
    "-g3"
    "-O0"
    "-MMD"
    "-MP"
    "-Wl,--specs-nosys.specs"
    "-Wl,-Map=stm32-ecu-manager.map"
    "-Wl,--gc-sections"
)

add_definitions("-DUSERSPACE_BUILD")

include_directories(
    ${ECU_SHARED_INCLUDE}
    "${CMAKE_BINARY_DIR}/install/include"
)

if(NOT TARGET ecu_shared)
  add_subdirectory("../../CM4/ecu-dev/src/shared" "shared")
endif()

## gRPC communications library
# After using add_subdirectory, we can now use the grpc targets directly from this build.

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

if(STM32_ECU_MANAGER_BUILD_SERVER)
  set(GRPC_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/grpc/cmake/build/install_arm)
  set(absl_DIR ${GRPC_BUILD_DIR}/lib/cmake/absl)
  set(Protobuf_DIR ${GRPC_BUILD_DIR}/lib/cmake/protobuf)
  set(gRPC_DIR ${GRPC_BUILD_DIR}/lib/cmake/grpc)
endif()

# This branch assumes that gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

if(STM32_ECU_MANAGER_BUILD_SERVER)
  find_program(_PROTOBUF_PROTOC protoc)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()


# Proto file
get_filename_component(ecu_proto "${CMAKE_CURRENT_SOURCE_DIR}/protos/ecu.proto" ABSOLUTE)
get_filename_component(ecu_proto_path "${ecu_proto}" PATH)

# Generated sources
set(ecu_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/ecu.pb.cc")
set(ecu_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ecu.pb.h")
set(ecu_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/ecu.grpc.pb.cc")
set(ecu_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ecu.grpc.pb.h")

add_custom_command(
      OUTPUT
        "${ecu_proto_srcs}"
        "${ecu_proto_hdrs}"
        "${ecu_grpc_srcs}"
        "${ecu_grpc_hdrs}"
      COMMAND
        "${_PROTOBUF_PROTOC}"
      ARGS
        --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${ecu_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${ecu_proto}"
      DEPENDS
        "${ecu_proto}"
)


# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# ecu_grpc_proto
add_library(ecu_grpc_proto
  ${ecu_grpc_srcs}
  ${ecu_grpc_hdrs}
  ${ecu_proto_srcs}
  ${ecu_proto_hdrs}
)

target_link_libraries(ecu_grpc_proto
  gRPC::grpc++
  gRPC::grpc++_reflection
  protobuf::libprotobuf
)


if(STM32_ECU_MANAGER_BUILD_SERVER)

  ## stm32-ecu-manager executable
  add_executable(stm32-ecu-manager
      ${CMAKE_CURRENT_SOURCE_DIR}/server/main.cpp
      ${SHARED_SRC}
  )

  target_link_libraries(stm32-ecu-manager
      ecu_shared
      ecu_grpc_proto

      gRPC::grpc++
      gRPC::grpc++_reflection
      protobuf::libprotobuf
  )

  # Update userspace application
  set(KERNEL_INSTALL_DIR ${CMAKE_BINARY_DIR}/install)

  install(
    TARGETS
      stm32-ecu-manager
    RUNTIME
    DESTINATION
      ${KERNEL_INSTALL_DIR}/home/root/
  )

endif()


if(STM32_ECU_MANAGER_BUILD_CLIENT)

  ## stm32-ecu-client executable
  add_executable(stm32-ecu-client
    ${CMAKE_CURRENT_SOURCE_DIR}/client/main.cpp
    ${SHARED_SRC}
  )

  target_link_libraries(stm32-ecu-client
      ecu_shared
      ecu_grpc_proto

      gRPC::grpc++
      gRPC::grpc++_reflection
      protobuf::libprotobuf
  )

endif()
