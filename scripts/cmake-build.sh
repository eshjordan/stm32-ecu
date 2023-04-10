#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
BUILD_DIR=${SCRIPT_DIR}/../build


mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR}

SOURCE_STM='unset LD_LIBRARY_PATH && source ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi'

# TODO: Don't require that code is already generated
echo "Building available targets for x86. Don't forget to generate code first!"

rm CMakeCache.txt
cmake -DBUILD_KERNEL=OFF ..
cmake --build . -j
cmake --install .

echo "Cross-compile kernel"

rm CMakeCache.txt
bash -c "${SOURCE_STM} && cmake -DBUILD_KERNEL=ON .."
bash -c "${SOURCE_STM} && cmake --build . --target kernel_install -j"
bash -c "${SOURCE_STM} && cmake --install ."

echo "Cross-compile all targets"
rm CMakeCache.txt
bash -c "${SOURCE_STM} && cmake -DBUILD_KERNEL=OFF .."
bash -c "${SOURCE_STM} && cmake --build . -j"
bash -c "${SOURCE_STM} && cmake --install ."

popd
