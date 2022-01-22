#!/bin/bash
set -ex

ROOT_DIR=$(dirname $(readlink -f $0))
GRPC_ARM_INSTALL_DIR=${ROOT_DIR}/grpc/build

# Install openssl (to use instead of boringssl)
# sudo apt update && sudo apt install -y libssl-dev gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf git

git clone -b v1.42.0 https://github.com/grpc/grpc.git ${ROOT_DIR}/grpc || true

pushd ${ROOT_DIR}/grpc

git submodule update --init --recursive -j $(nproc)

# Build and install gRPC for the host architecture.
# We do this because we need to be able to run protoc and grpc_cpp_plugin
# while cross-compiling.
mkdir -p "cmake/build"
pushd "cmake/build"
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_TESTS=OFF \
  -DgRPC_SSL_PROVIDER=package \
  ../..
make "-j$(nproc)"
sudo make install
popd

# Write a toolchain file to use for cross-compiling.
echo "SET(CMAKE_SYSTEM_NAME Linux)" >/tmp/toolchain.cmake
echo "SET(CMAKE_SYSTEM_PROCESSOR arm)" >>/tmp/toolchain.cmake
echo "set(CMAKE_STAGING_PREFIX ${GRPC_ARM_INSTALL_DIR})" >>/tmp/toolchain.cmake
echo "set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)" >>/tmp/toolchain.cmake
echo "set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)" >>/tmp/toolchain.cmake
echo "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >>/tmp/toolchain.cmake
echo "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >>/tmp/toolchain.cmake
echo "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >>/tmp/toolchain.cmake
echo "set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)" >>/tmp/toolchain.cmake

# Build and install gRPC for ARM.
# This build will use the host architecture copies of protoc and
# grpc_cpp_plugin that we built earlier because we installed them
# to a location in our PATH (/usr/local/bin).
mkdir -p "cmake/build_arm"
pushd "cmake/build_arm"
cmake -DCMAKE_TOOLCHAIN_FILE=/tmp/toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=${GRPC_ARM_INSTALL_DIR} \
  ../..
make "-j$(nproc)"
make install
popd

# Build helloworld example for ARM.
# As above, it will find and use protoc and grpc_cpp_plugin
# for the host architecture.
mkdir -p "examples/cpp/helloworld/cmake/build_arm"
pushd "examples/cpp/helloworld/cmake/build_arm"
cmake -DCMAKE_TOOLCHAIN_FILE=/tmp/toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -Dabsl_DIR=/tmp/stage/lib/cmake/absl \
  -DProtobuf_DIR=/tmp/stage/lib/cmake/protobuf \
  -DgRPC_DIR=${GRPC_ARM_INSTALL_DIR}/lib/cmake/grpc \
  ../..
make "-j$(nproc)"
popd

popd
