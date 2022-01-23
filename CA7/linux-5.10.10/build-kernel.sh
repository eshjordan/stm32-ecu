#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
KERNEL_VERSION=5.10.10
SOURCE_DIR=${SCRIPT_DIR}/linux-${KERNEL_VERSION}
BUILD_DIR=${SCRIPT_DIR}/build
KERNEL_SOURCE_DIR=/home/jordan/Documents/2021/stm32-resources/Developer-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-03-31/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.10-r0
BOARD_IP=192.168.10.129
BOOTFS=/dev/mmcblk0p4


POSITIONAL=()
while [[ $# -gt 0 ]]; do
  key="$1"

  case $key in
    -f|--full)
      BUILD="true"
      LOAD="true"
      REBUILD="true"
      shift # past argument
      ;;
    -b|--build)
      BUILD="true"
      shift # past argument
      ;;
    -r|--rebuild)
      REBUILD="true"
      shift # past argument
      ;;
    -l|--load)
      LOAD="true"
      shift # past argument
      ;;
    *)    # unknown option
      POSITIONAL+=("$1") # save it in an array for later
      shift # past argument
      ;;
  esac
done

set -- "${POSITIONAL[@]}" # restore positional parameters

if [ "${BUILD}" != "true" ] && [ "${LOAD}" != "true" ] && [ "${REBUILD}" != "true" ]; then
  echo "No action specified. Use -f, -b, -r or -l"
  exit 1
fi

# Source the SDK environment for cross-compilation
source /opt/st/stm32mp1/3.1-openstlinux-5.10-dunfell-mp1-21-03-31/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi

if [ "${BUILD}" = "true" ]; then
    echo "Reconfiguring and patching kernel"

    cd ${SOURCE_DIR}

    # Prepare kernel source
    tar xfJ ${KERNEL_SOURCE_DIR}/linux-5.10.10.tar.xz -C ${SCRIPT_DIR}

    # Patch kernel sources
    for p in `ls -1 ${KERNEL_SOURCE_DIR}/*.patch`; do patch -p1 -N < $p; done

    # Configure build directory
    mkdir -p ${BUILD_DIR}
    make ARCH=arm O="${BUILD_DIR}" multi_v7_defconfig fragment*.config -j$(nproc)

    # Apply fragments
    for f in `ls -1 ${SCRIPT_DIR}/arch/arm/configs/fragment*.config`; do ${SOURCE_DIR}/scripts/kconfig/merge_config.sh -m -r -O ${BUILD_DIR} ${BUILD_DIR}/.config $f; done
    for f in `ls -1 ${KERNEL_SOURCE_DIR}/fragment*.config`; do ${SOURCE_DIR}/scripts/kconfig/merge_config.sh -m -r -O ${BUILD_DIR} ${BUILD_DIR}/.config $f; done
    yes '' | make ARCH=arm oldconfig O="${BUILD_DIR}" -j$(nproc)
fi

if [ "${REBUILD}" = "true" ]; then
    cd ${SOURCE_DIR}

    # Build kernel images (uImage and vmlinux) and device tree (dtbs)
    make ARCH=arm uImage vmlinux dtbs LOADADDR=0xC2000040 O="${BUILD_DIR}" -j$(nproc)

    # Build kernel module
    make ARCH=arm all O="${BUILD_DIR}" -j$(nproc)

    # Generate compile_commands.json
    ${SCRIPT_DIR}/linux-5.10.10/scripts/clang-tools/gen_compile_commands.py
fi

if [ "${LOAD}" = "true" ]; then
    echo "Updating kernel, device tree and modules"

    cd ${SOURCE_DIR}

    # Generate userspace headers
    make ARCH=arm INSTALL_HDR_PATH="${BUILD_DIR}/install_artifact" headers_install O="${BUILD_DIR}" -j$(nproc)

    # Generate output build artifacts
    make ARCH=arm INSTALL_MOD_PATH="${BUILD_DIR}/install_artifact" modules_install O="${BUILD_DIR}" -j$(nproc)
    mkdir -p ${BUILD_DIR}/install_artifact/boot/
    rsync -rvcaz ${BUILD_DIR}/arch/arm/boot/uImage ${BUILD_DIR}/install_artifact/boot/
    rsync -rvcaz ${BUILD_DIR}/arch/arm/boot/dts/st*.dtb ${BUILD_DIR}/install_artifact/boot/

    # Update kernel and device tree
    cd ${BUILD_DIR}/install_artifact
    ssh root@${BOARD_IP} "apt-get update -q && apt-get install -yq rsync"
    ssh root@${BOARD_IP} mount ${BOOTFS} /boot
    # ssh root@${BOARD_IP} "rm -rf /boot/*.dtb; rm -rf /boot/uImage; rm -rf /boot/vmlinux"
    rsync -rvcazC ${BUILD_DIR}/install_artifact/boot/* root@${BOARD_IP}:/boot/
    ssh root@${BOARD_IP} umount /boot

    # Update kernel modules
    # find ${BUILD_DIR}/install_artifact -name "*.ko" | xargs $STRIP --strip-debug --remove-section=.comment --remove-section=.note --preserve-dates
    echo "${KERNEL_VERSION}/source ${KERNEL_VERSION}/build" > ${BUILD_DIR}/install_artifact/lib/modules/.cvsignore
    # ssh root@${BOARD_IP} "rm -rf /lib/modules/*"
    rsync -rvcazC ${BUILD_DIR}/install_artifact/lib/modules/* root@${BOARD_IP}:/lib/modules/

    # Generate a list of module dependencies (modules.dep) and a list of symbols provided by modules (modules.symbols)
    ssh root@${BOARD_IP} /sbin/depmod -a

    # Synchronize data on disk with memory
    ssh root@${BOARD_IP} sync

    # Reboot the board in order to take update into account
    ssh root@${BOARD_IP} /sbin/reboot
fi
