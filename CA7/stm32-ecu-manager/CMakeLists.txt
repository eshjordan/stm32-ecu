# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake "superbuild" file for C++ helloworld example.
# This build file demonstrates how to build the helloworld project
# and all its dependencies in a single cmake build (hence "superbuild")
# that is easy to build and maintain.
# cmake's ExternalProject_Add() is used to import all the sub-projects,
# including the "helloworld" project itself.
# See https://blog.kitware.com/cmake-superbuilds-git-submodules/

cmake_minimum_required(VERSION 3.5.1)

# Project
project(stm32-ecu-manager C CXX)

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

include_directories(
    ${ECU_SHARED_INCLUDE}
    "../linux-5.10.61/build/install_artifact/include"
)

add_subdirectory("shared")

## gRPC communications library
# After using add_subdirectory, we can now use the grpc targets directly from this build.

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

if(BUILD_SERVER)
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

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(BUILD_SERVER)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(BUILD_SERVER)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# add_subdirectory("grpc")

# find_program(_PROTOBUF_PROTOC protoc)
# find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
# set(_PROTOBUF_PROTOC "${CMAKE_CURRENT_BINARY_DIR}/grpc/third_party/protobuf/protoc")
# set(_GRPC_CPP_PLUGIN_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/grpc/grpc_cpp_plugin")

# Proto file
get_filename_component(ecu_proto "protos/ecu.proto" ABSOLUTE)
get_filename_component(ecu_proto_path "${ecu_proto}" PATH)

# Generated sources
set(ecu_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/ecu.pb.cc")
set(ecu_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ecu.pb.h")
set(ecu_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/ecu.grpc.pb.cc")
set(ecu_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ecu.grpc.pb.h")
add_custom_command(
      OUTPUT "${ecu_proto_srcs}" "${ecu_proto_hdrs}" "${ecu_grpc_srcs}" "${ecu_grpc_hdrs}"
      COMMAND "${_PROTOBUF_PROTOC}"
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${ecu_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${ecu_proto}"
      DEPENDS "${ecu_proto}")

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
  ${_GRPC_GRPCPP}
  ${_REFLECTION}
  ${_PROTOBUF_LIBPROTOBUF}
)


if(BUILD_SERVER)

  ## stm32-ecu-manager executable
  add_executable(stm32-ecu-manager
      server/main.cpp
      ${SHARED_SRC}
  )

  target_link_libraries(stm32-ecu-manager
      ecu_shared
      ecu_grpc_proto
      ${_GRPC_GRPCPP}
      ${_REFLECTION}
      ${_PROTOBUF_LIBPROTOBUF}
  )

  add_custom_command(
    TARGET stm32-ecu-manager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
  )

endif()


if(BUILD_CLIENT)

  ## stm32-ecu-client executable
  add_executable(stm32-ecu-client
      client/main.cpp
      # ${SHARED_SRC}
  )

  target_link_libraries(stm32-ecu-client
      # ecu_shared
      ecu_grpc_proto
      ${_GRPC_GRPCPP}
      ${_REFLECTION}
      ${_PROTOBUF_LIBPROTOBUF}
  )

  add_custom_command(
    TARGET stm32-ecu-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
  )

endif()
