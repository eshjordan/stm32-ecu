#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
BUILD_DIR=${SCRIPT_DIR}/../build


mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR}

# Build available targets for x86
cmake -DBUILD_KERNEL=OFF ..
cmake --build . -j
cmake --install .

# Cross-compile kernel
unset LD_LIBRARY_PATH
source ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
cmake -DBUILD_KERNEL=ON ..
cmake --build . --target kernel_install -j
cmake --install .

# Cross-compile all targets
rm CMakeCache.txt
cmake -DBUILD_KERNEL=OFF ..
cmake --build . -j
cmake --install .

popd
