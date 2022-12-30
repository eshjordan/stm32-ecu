#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
KERNEL_VERSION=5.10.61
KERNEL_ROOT_DIR=${SCRIPT_DIR}/../CA7/linux-${KERNEL_VERSION}
KERNEL_SOURCE_DIR=${KERNEL_ROOT_DIR}/linux-${KERNEL_VERSION}
KERNEL_BUILD_DIR=${KERNEL_ROOT_DIR}/build
KERNEL_ZIP_DIR=${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.61-stm32mp-r2-r0

BOARD_IP=192.168.0.4
BOOTFS=/dev/mmcblk0p4

POSITIONAL=()
while [[ $# -gt 0 ]]; do
    key="$1"

    case $key in
    -f | --full)
        BUILD="true"
        LOAD="true"
        REBUILD="true"
        shift # past argument
        ;;
    -b | --build)
        BUILD="true"
        shift # past argument
        ;;
    -r | --rebuild)
        REBUILD="true"
        shift # past argument
        ;;
    -l | --load)
        LOAD="true"
        shift # past argument
        ;;
    *)                     # unknown option
        POSITIONAL+=("$1") # save it in an array for later
        shift              # past argument
        ;;
    esac
done

set -- "${POSITIONAL[@]}" # restore positional parameters

if [ "${BUILD}" != "true" ] && [ "${LOAD}" != "true" ] && [ "${REBUILD}" != "true" ]; then
    echo "No action specified. Use -f, -b, -r or -l"
    exit 1
fi

# Source the SDK environment for cross-compilation
source ${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi

if [ "${BUILD}" = "true" ]; then
    echo "Reconfiguring and patching kernel"

    mkdir -p ${KERNEL_SOURCE_DIR}
    cd ${KERNEL_SOURCE_DIR}

    # Prepare kernel source
    tar xfJ ${KERNEL_ZIP_DIR}/linux-${KERNEL_VERSION}.tar.xz -C ${KERNEL_ROOT_DIR}

    # Patch kernel sources
    for p in $(ls -1 ${KERNEL_ZIP_DIR}/*.patch); do patch -p1 -N <$p; done
    for p in $(ls -1 ${KERNEL_ROOT_DIR}/../CustomKernel/patches/*.patch); do patch -p0 -N <$p; done

    # Configure build directory
    mkdir -p ${KERNEL_BUILD_DIR}
    make ARCH=arm O="${KERNEL_BUILD_DIR}" multi_v7_defconfig fragment*.config -j$(nproc)

    # Apply fragments
    for f in $(ls -1 ${KERNEL_SOURCE_DIR}/arch/arm/configs/fragment*.config); do ${KERNEL_SOURCE_DIR}/scripts/kconfig/merge_config.sh -m -r -O ${KERNEL_BUILD_DIR} ${KERNEL_BUILD_DIR}/.config $f; done
    for f in $(ls -1 ${KERNEL_ZIP_DIR}/fragment*.config); do ${KERNEL_SOURCE_DIR}/scripts/kconfig/merge_config.sh -m -r -O ${KERNEL_BUILD_DIR} ${KERNEL_BUILD_DIR}/.config $f; done
    yes '' | make ARCH=arm oldconfig O="${KERNEL_BUILD_DIR}" -j$(nproc)
fi

if [ "${REBUILD}" = "true" ]; then
    cd ${KERNEL_SOURCE_DIR}

    # Build kernel images (uImage and vmlinux) and device tree (dtbs)
    make ARCH=arm uImage vmlinux dtbs LOADADDR=0xC2000040 O="${KERNEL_BUILD_DIR}" -j$(nproc)

    # Build kernel module
    make ARCH=arm all O="${KERNEL_BUILD_DIR}" -j$(nproc)

    # Build userspace application
    mkdir -p ${KERNEL_ROOT_DIR}/../stm32-ecu-manager/Default
    cd ${KERNEL_ROOT_DIR}/../stm32-ecu-manager/Default
    cmake ..
    make -j$(nproc)

    # Generate compile_commands.json
    ${SCRIPT_DIR}/combine_compile_commands.sh
fi

if [ "${LOAD}" = "true" ]; then
    echo "Updating kernel, device tree, headers, modules, firmware and userspace applications"

    cd ${KERNEL_SOURCE_DIR}

    # Generate userspace headers
    make ARCH=arm INSTALL_HDR_PATH="${KERNEL_BUILD_DIR}/install_artifact" headers_install O="${KERNEL_BUILD_DIR}" -j$(nproc)

    # Generate kernel modules
    make ARCH=arm INSTALL_MOD_PATH="${KERNEL_BUILD_DIR}/install_artifact" modules_install O="${KERNEL_BUILD_DIR}" -j$(nproc)

    # Copy generated boot image and device tree
    mkdir -p ${KERNEL_BUILD_DIR}/install_artifact/boot/
    rsync -rvcaz ${KERNEL_BUILD_DIR}/arch/arm/boot/uImage ${KERNEL_BUILD_DIR}/install_artifact/boot/
    rsync -rvcaz ${KERNEL_BUILD_DIR}/arch/arm/boot/dts/st*.dtb ${KERNEL_BUILD_DIR}/install_artifact/boot/

    # Add other custom rootfs files to install artifacts
    rsync -rvcaz ${KERNEL_ROOT_DIR}/../rootfs/ ${KERNEL_BUILD_DIR}/install_artifact/

    rsync -rvcaz ${KERNEL_ROOT_DIR}/../../CM4/ProdDebug/stm32-ecu_CM4.elf ${KERNEL_BUILD_DIR}/install_artifact/home/root/stm32-ecu_CM4.elf | true
    rsync -rvcaz ${KERNEL_ROOT_DIR}/../stm32-ecu-manager/Default/stm32-ecu-manager ${KERNEL_BUILD_DIR}/install_artifact/home/root/stm32-ecu-manager | true

    # Make sure rsync is installed
    ssh root@${BOARD_IP} "apt-get update -q && apt-get install -yq rsync"

    cd ${KERNEL_BUILD_DIR}/install_artifact

    # Ignore source and build dirs
    echo "${KERNEL_VERSION}/source ${KERNEL_VERSION}/build" >${KERNEL_BUILD_DIR}/install_artifact/lib/modules/.cvsignore

    # Strip debug symbols
    # find ${KERNEL_BUILD_DIR}/install_artifact -name "*.ko" | xargs $STRIP --strip-debug --remove-section=.comment --remove-section=.note --preserve-dates

    # Remove all modules on the target
    # ssh root@${BOARD_IP} "rm -rf /lib/modules/*"

    # Copy all install artifacts to the target
    ssh root@${BOARD_IP} umount ${BOOTFS}
    ssh root@${BOARD_IP} mount ${BOOTFS} /boot
    # ssh root@${BOARD_IP} "rm -rf /boot/*.dtb; rm -rf /boot/uImage; rm -rf /boot/vmlinux"
    # ssh root@${BOARD_IP} "rm -rf /home/root; rm -rf /include; rm -rf /lib/modules"
    # rsync -rvcazC ${KERNEL_BUILD_DIR}/install_artifact/* root@${BOARD_IP}:/

    ssh root@${BOARD_IP} "chown --silent -R root:root /"

    ssh root@${BOARD_IP} umount /boot

    # Generate a list of module dependencies (modules.dep) and a list of symbols provided by modules (modules.symbols)
    ssh root@${BOARD_IP} /sbin/depmod -a

    # Synchronize data on disk with memory
    ssh root@${BOARD_IP} sync

    # Reboot the board in order to take update into account
    ssh root@${BOARD_IP} /sbin/reboot
fi
