#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
KERNEL_VERSION=5.10.10
ROOT_DIR=${SCRIPT_DIR}/../CA7/linux-${KERNEL_VERSION}
SOURCE_DIR=${ROOT_DIR}/linux-${KERNEL_VERSION}
BUILD_DIR=${ROOT_DIR}/build
KERNEL_SOURCE_DIR=/home/jordan/Documents/2021/stm32-resources/Developer-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-03-31/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.10-r0
BOARD_IP=192.168.10.129
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

# Source the SDK environment for cross-compilation
source /opt/st/stm32mp1/3.1-openstlinux-5.10-dunfell-mp1-21-03-31/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi

# Build userspace application
mkdir -p ${ROOT_DIR}/../stm32-ecu-manager/Default
cd ${ROOT_DIR}/../stm32-ecu-manager/Default
cmake ..
make -j$(nproc)

if [ "${FIRMWARE_ONLY}" = "true" ]; then

    # Update CM4 firmware
    rsync -rvcaz ${ROOT_DIR}/../../CM4/ProdDebug/stm32-ecu_CM4.elf root@${BOARD_IP}:/home/root/stm32-ecu_CM4.elf | true

    # Update userspace application
    rsync -rvcaz ${ROOT_DIR}/../stm32-ecu-manager/Default/stm32-ecu-manager root@${BOARD_IP}:/home/root/stm32-ecu-manager | true

    ssh root@${BOARD_IP} "chown --silent root:root \
/home/root/stm32-ecu_CM4.elf \
/home/root/stm32-ecu-manager \
"

else

    echo "Updating kernel, device tree, headers, modules, firmware and userspace applications"

    # Ensure kernel modules are built
    cd ${SOURCE_DIR}
    make ARCH=arm modules O="${BUILD_DIR}" -j$(nproc)

    # Generate compile_commands.json
    ${SCRIPT_DIR}/combine_compile_commands.sh

    # Update boot image and device tree
    ssh root@${BOARD_IP} mount ${BOOTFS} /boot

    mkdir -p ${BUILD_DIR}/install_artifact/boot/
    rsync -rvcaz ${BUILD_DIR}/arch/arm/boot/uImage root@${BOARD_IP}:/boot/
    rsync -rvcaz ${BUILD_DIR}/arch/arm/boot/dts/st*.dtb root@${BOARD_IP}:/boot/

    # Update userspace headers
    rsync -rvcazC ${SOURCE_DIR}/include/uapi/linux/stm32ecu/ root@${BOARD_IP}:/include/linux/stm32ecu/

    # Update kernel module
    rsync -rvcazC ${ROOT_DIR}/build/drivers/stm32ecu/stm32ecu.ko root@${BOARD_IP}:/lib/modules/${KERNEL_VERSION}/kernel/drivers/stm32ecu/stm32ecu.ko

    # Update CM4 firmware
    rsync -rvcaz ${ROOT_DIR}/../../CM4/ProdDebug/stm32-ecu_CM4.elf root@${BOARD_IP}:/home/root/stm32-ecu_CM4.elf | true

    # Update userspace application
    rsync -rvcaz ${ROOT_DIR}/../stm32-ecu-manager/Default/stm32-ecu-manager root@${BOARD_IP}:/home/root/stm32-ecu-manager | true

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
