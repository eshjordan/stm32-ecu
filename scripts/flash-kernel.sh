#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
KERNEL_VERSION=5.10.61
KERNEL_ROOT_DIR=${SCRIPT_DIR}/../CA7/linux-${KERNEL_VERSION}
KERNEL_SOURCE_DIR=${KERNEL_ROOT_DIR}/linux-${KERNEL_VERSION}
KERNEL_BUILD_DIR=${KERNEL_ROOT_DIR}/build
KERNEL_INSTALL_DIR=${SCRIPT_DIR}/../build/install
KERNEL_ZIP_DIR=${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.61-stm32mp-r2-r0

BOARD_IP=192.168.0.4
BOOTFS=/dev/mmcblk0p4

POSITIONAL=()
while [[ $# -gt 0 ]]; do
    key="$1"

    case $key in
    -f | --full)
        FULL=true
        shift # past argument
        ;;
    *)                     # unknown option
        POSITIONAL+=("$1") # save it in an array for later
        shift              # past argument
        ;;
    esac
done

set -- "${POSITIONAL[@]}" # restore positional parameters


# Make sure rsync is installed
ssh root@${BOARD_IP} "apt-get update -q"
ssh root@${BOARD_IP} "apt-get install -yq rsync"


if [ $FULL ];
then

    # Mount boot partition
    ssh root@${BOARD_IP} "umount ${BOOTFS}"
    ssh root@${BOARD_IP} "mount ${BOOTFS} /boot"

    # # Remove all modules on the target
    # ssh root@${BOARD_IP} "rm -rf /lib/modules/*"
    # ssh root@${BOARD_IP} "rm -rf /boot/*.dtb; rm -rf /boot/uImage; rm -rf /boot/vmlinux"
    # ssh root@${BOARD_IP} "rm -rf /home/root; rm -rf /include; rm -rf /lib/modules"

    # Copy all install artifacts to the target
    rsync -rvcazC ${KERNEL_INSTALL_DIR}/* root@${BOARD_IP}:/
    ssh root@${BOARD_IP} "chown --silent -R root:root /"

    # Unmount boot partition
    ssh root@${BOARD_IP} "umount /boot"

else

    # Update userspace headers
    rsync -rvcazC ${KERNEL_INSTALL_DIR}/include/linux/stm32ecu/ root@${BOARD_IP}:/include/linux/stm32ecu/
    ssh root@${BOARD_IP} "chown --silent -R root:root /include/linux/stm32ecu/"

    # Update kernel module
    rsync -rvcazC ${KERNEL_INSTALL_DIR}/lib/modules/${KERNEL_VERSION}/kernel/drivers/stm32ecu/stm32ecu.ko root@${BOARD_IP}:/lib/modules/${KERNEL_VERSION}/kernel/drivers/stm32ecu/stm32ecu.ko
    ssh root@${BOARD_IP} "chown --silent -R root:root /lib/modules/${KERNEL_VERSION}/kernel/drivers/stm32ecu/stm32ecu.ko"

    # Update CM4 firmware
    rsync -rvcazC ${KERNEL_INSTALL_DIR}/home/root/stm32-ecu_CM4.elf root@${BOARD_IP}:/home/root/stm32-ecu_CM4.elf | true
    ssh root@${BOARD_IP} "chown --silent -R root:root /home/root/stm32-ecu_CM4.elf"

    # Update userspace application
    rsync -rvcazC ${KERNEL_INSTALL_DIR}/home/root/stm32-ecu-manager root@${BOARD_IP}:/home/root/stm32-ecu-manager | true
    ssh root@${BOARD_IP} "chown --silent -R root:root /home/root/stm32-ecu-manager"

fi

# Generate a list of module dependencies (modules.dep) and a list of symbols provided by modules (modules.symbols)
ssh root@${BOARD_IP} "/sbin/depmod -a"

# Synchronize data on disk with memory
ssh root@${BOARD_IP} "sync"

# Reboot the board in order to take update into account
ssh root@${BOARD_IP} "/sbin/reboot"
