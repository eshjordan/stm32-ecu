#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
WORKSPACE_DIR=${SCRIPT_DIR}/..
STM32_MANAGER_DIR=${WORKSPACE_DIR}/CA7/stm32-ecu-manager
KERNEL_VERSION=5.10.61
KERNEL_ROOT_DIR=${WORKSPACE_DIR}/CA7/linux-${KERNEL_VERSION}
KERNEL_SOURCE_DIR=${KERNEL_ROOT_DIR}/linux-${KERNEL_VERSION}
KERNEL_BUILD_DIR=${KERNEL_ROOT_DIR}/build
KERNEL_ZIP_DIR=${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.61-stm32mp-r2-r0

BOARD_IP=192.168.0.4
BOOTFS=/dev/mmcblk0p4

POSITIONAL=()
while [[ $# -gt 0 ]]; do
    key="$1"

    case $key in
    -f | --firmware)
        FIRMWARE_ONLY="true"
        shift # past argument
        ;;
    *)                     # unknown option
        POSITIONAL+=("$1") # save it in an array for later
        shift              # past argument
        ;;
    esac
done

set -- "${POSITIONAL[@]}" # restore positional parameters


# Build project for x86.
cmake -S ${WORKSPACE_DIR} -B ${WORKSPACE_DIR}/build/x86
cmake --build ${WORKSPACE_DIR}/build/x86 -j

popd


# Source the SDK environment for cross-compilation
unset LD_LIBRARY_PATH
source ${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi

# Build project for ARM
cmake -S ${WORKSPACE_DIR} -B ${WORKSPACE_DIR}/build/arm
cmake --build ${WORKSPACE_DIR}/build/arm -j

popd

# # Write a toolchain file to use for cross-compiling.
# echo "SET(CMAKE_SYSTEM_NAME Linux)" >/tmp/toolchain.cmake
# echo "SET(CMAKE_SYSTEM_PROCESSOR arm)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_C_COMPILE_FLAGS '\${CMAKE_C_COMPILE_FLAGS} -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a7')" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_CXX_COMPILE_FLAGS '\${CMAKE_CXX_COMPILE_FLAGS} -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a7')" >>/tmp/toolchain.cmake
# echo "SET(CMAKE_LINKER_FLAGS '\${CMAKE_LINKER_FLAGS}')" >>/tmp/toolchain.cmake

# cmake \
#     -DCMAKE_TOOLCHAIN_FILE=/tmp/toolchain.cmake \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DBUILD_SERVER=ON \
#     -DBUILD_CLIENT=OFF \
#     ..


if [ "${FIRMWARE_ONLY}" = "true" ]; then

    # Update CM4 firmware
    rsync -rvcaz ${KERNEL_ROOT_DIR}/../../CM4/ProdDebug/stm32-ecu_CM4.elf root@${BOARD_IP}:/home/root/stm32-ecu_CM4.elf | true

    # Update userspace application
    rsync -rvcaz ${KERNEL_ROOT_DIR}/../stm32-ecu-manager/build/server/stm32-ecu-manager root@${BOARD_IP}:/home/root/stm32-ecu-manager | true

    ssh root@${BOARD_IP} "chown --silent root:root \
/home/root/stm32-ecu_CM4.elf \
/home/root/stm32-ecu-manager \
"

else

    echo "Updating kernel, device tree, headers, modules, firmware and userspace applications"

    # Ensure kernel modules are built
    cd ${KERNEL_SOURCE_DIR}
    make ARCH=arm modules O="${KERNEL_BUILD_DIR}" -j$(nproc)

    # Generate compile_commands.json
    ${SCRIPT_DIR}/combine_compile_commands.sh

    # Update boot image and device tree
    ssh root@${BOARD_IP} mount ${BOOTFS} /boot

    mkdir -p ${KERNEL_BUILD_DIR}/install_artifact/boot/
    rsync -rvcaz ${KERNEL_BUILD_DIR}/arch/arm/boot/uImage root@${BOARD_IP}:/boot/
    rsync -rvcaz ${KERNEL_BUILD_DIR}/arch/arm/boot/dts/st*.dtb root@${BOARD_IP}:/boot/

    # Update userspace headers
    rsync -rvcazC ${KERNEL_SOURCE_DIR}/include/uapi/linux/stm32ecu/ root@${BOARD_IP}:/include/linux/stm32ecu/

    # Update kernel module
    rsync -rvcazC ${KERNEL_ROOT_DIR}/build/drivers/stm32ecu/stm32ecu.ko root@${BOARD_IP}:/lib/modules/${KERNEL_VERSION}/kernel/drivers/stm32ecu/stm32ecu.ko

    # Update CM4 firmware
    rsync -rvcaz ${KERNEL_ROOT_DIR}/../../CM4/ProdDebug/stm32-ecu_CM4.elf root@${BOARD_IP}:/home/root/stm32-ecu_CM4.elf | true

    # Update userspace application
    rsync -rvcaz ${KERNEL_ROOT_DIR}/../stm32-ecu-manager/build/server/stm32-ecu-manager root@${BOARD_IP}:/home/root/stm32-ecu-manager | true

    ssh root@${BOARD_IP} "chown --silent -R root:root \
/boot/uImage \
/boot/st*.dtb \
/lib/modules/${KERNEL_VERSION}/kernel/drivers/stm32ecu/stm32ecu.ko \
/include/linux/stm32ecu/
/home/root/stm32-ecu_CM4.elf \
/home/root/stm32-ecu-manager \
"

    ssh root@${BOARD_IP} umount /boot

    # Generate a list of module dependencies (modules.dep) and a list of symbols provided by modules (modules.symbols)
    ssh root@${BOARD_IP} /sbin/depmod -a

    # Synchronize data on disk with memory
    ssh root@${BOARD_IP} sync

    # Reboot the board in order to take update into account
    ssh root@${BOARD_IP} /sbin/reboot

fi

exit 0
