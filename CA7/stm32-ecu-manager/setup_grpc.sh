#!/bin/bash

set -ex

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
GRPC_INSTALL_DIR=${SCRIPT_DIR}/grpc/build
STM_SYSROOT_DIR=${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/sysroots
STM_TOOLCHAIN_DIR=${STM_SYSROOT_DIR}/x86_64-ostl_sdk-linux/usr/bin/arm-ostl-linux-gnueabi
JOBS=16

pushd ${SCRIPT_DIR}
# Install openssl (to use instead of boringssl)
sudo apt update && sudo apt install -y libssl-dev git cmake gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

git clone -b v1.42.0 https://github.com/grpc/grpc.git ${SCRIPT_DIR}/grpc || true

pushd ${SCRIPT_DIR}/grpc

git submodule update --init --recursive -j ${JOBS}

# Build and install gRPC for the host architecture.
# We do this because we need to be able to run protoc and grpc_cpp_plugin
# while cross-compiling.
mkdir -p ${SCRIPT_DIR}/grpc/cmake/build
pushd ${SCRIPT_DIR}/grpc/cmake/build
cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DgRPC_SSL_PROVIDER=package \
      ../..
make -j${JOBS}
sudo make install
popd


GRPC_INSTALL_DIR=${SCRIPT_DIR}/grpc/cmake/build/install_arm

# # Write a toolchain file to use for cross-compiling.
# echo "SET(CMAKE_SYSTEM_NAME Linux)" >/tmp/toolchain.cmake
# echo "SET(CMAKE_SYSTEM_PROCESSOR arm)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_STAGING_PREFIX ${GRPC_INSTALL_DIR})" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_C_COMPILE_FLAGS '\${CMAKE_C_COMPILE_FLAGS} -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a7')" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_CXX_COMPILE_FLAGS '\${CMAKE_CXX_COMPILE_FLAGS} -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a7')" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_LINKER_FLAGS '\${CMAKE_LINKER_FLAGS}')" >>/tmp/toolchain.cmake

# Build and install gRPC for ARM.
# This build will use the host architecture copies of protoc and
# grpc_cpp_plugin that we built earlier because we installed them
# to a location in our PATH (/usr/local/bin).
# unset LD_LIBRARY_PATH
# source ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
mkdir -p ${SCRIPT_DIR}/grpc/cmake/build/build_arm
pushd ${SCRIPT_DIR}/grpc/cmake/build/build_arm
cmake \
      -DCMAKE_TOOLCHAIN_FILE=/tmp/toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=${GRPC_INSTALL_DIR} \
      ../../..
make -j${JOBS} install
popd


popd
