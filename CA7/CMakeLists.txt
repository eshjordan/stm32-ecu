cmake_minimum_required(VERSION 3.0)
project(stm32-ecu-linux)


### Configuration

# One of EngDebug, EngRelease, ProdDebug, ProdRelease
if(NOT DEFINED CM4_CONFIG)
    set(CM4_CONFIG "ProdDebug")
endif()


### Build settings

include(ProcessorCount)
ProcessorCount(num_jobs)
math(EXPR num_jobs "${num_jobs}-3")

set(KERNEL_VERSION 5.10.61)
set(KERNEL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/linux-${KERNEL_VERSION})
set(KERNEL_SOURCE_DIR ${KERNEL_ROOT_DIR}/linux-${KERNEL_VERSION})
set(KERNEL_BUILD_DIR ${KERNEL_ROOT_DIR}/build)
set(KERNEL_ZIP_DIR $ENV{HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.61-stm32mp-r2-r0)


# Patch and configure kernel sources
message(STATUS "Reconfiguring and patching kernel")

# Unzip kernel sources
add_custom_target(kernel_src_unzip
    COMMAND
        tar xfJ ${KERNEL_ZIP_DIR}/linux-${KERNEL_VERSION}.tar.xz -C ${KERNEL_ROOT_DIR}
)

# Copy custom kernel files to the build tree
add_custom_target(custom_kernel_module
    COMMAND
        cp -TRf ${CMAKE_CURRENT_LIST_DIR}/CustomKernel/drivers/stm32ecu/ ${KERNEL_SOURCE_DIR}/drivers/stm32ecu/
    COMMAND
        cp -TRf ${CMAKE_CURRENT_LIST_DIR}/CustomKernel/include/uapi/linux/stm32ecu/ ${KERNEL_SOURCE_DIR}/include/uapi/linux/stm32ecu/
    # Remove the symlink used for development, copy the actual header files
    COMMAND
        rm ${KERNEL_SOURCE_DIR}/include/uapi/linux/stm32ecu/shared
    COMMAND
        mkdir -p ${KERNEL_SOURCE_DIR}/include/uapi/linux/stm32ecu/shared/
    COMMAND
        cp -f ${CMAKE_CURRENT_LIST_DIR}/../CM4/ecu-dev/src/shared/include/* ${KERNEL_SOURCE_DIR}/include/uapi/linux/stm32ecu/shared/
    DEPENDS
        kernel_src_unzip
)

set(prev_target kernel_src_unzip)

# Patch and configure kernel sources
file(GLOB patch_files
    ${KERNEL_ZIP_DIR}/*.patch
    ${CMAKE_CURRENT_LIST_DIR}/CustomKernel/patches/*.patch
)
list(SORT patch_files)

foreach(p ${patch_files})

    get_filename_component(filename ${p} NAME_WE)
    add_custom_target(kernel_patch_${filename}
        WORKING_DIRECTORY
            ${KERNEL_SOURCE_DIR}
        COMMAND
            sh -c "patch -t -p1 -N <${p}" || (exit 0)
        DEPENDS
            custom_kernel_module
            ${prev_target}
    )

    set(prev_target kernel_patch_${filename})

endforeach()


# Configure build directory
add_custom_target(kernel_src_configure
    WORKING_DIRECTORY
        ${KERNEL_SOURCE_DIR}
    COMMAND
        mkdir -p ${KERNEL_BUILD_DIR}
    COMMAND
        ${CMAKE_MAKE_PROGRAM} ARCH=arm O="${KERNEL_BUILD_DIR}" multi_v7_defconfig fragment*.config -j${num_jobs}
    DEPENDS
        ${prev_target}
)
set(prev_target kernel_src_configure)


# Apply fragments
file(GLOB config_files
    ${KERNEL_SOURCE_DIR}/arch/arm/configs/fragment*.config
    ${KERNEL_ZIP_DIR}/fragment*.config
)
list(SORT config_files)


foreach(f ${config_files})

    get_filename_component(filename ${f} NAME_WE)
    add_custom_target(kernel_config_${filename}
        WORKING_DIRECTORY
            ${KERNEL_SOURCE_DIR}
        COMMAND
            sh -c "${KERNEL_SOURCE_DIR}/scripts/kconfig/merge_config.sh -m -r -O ${KERNEL_BUILD_DIR} ${KERNEL_BUILD_DIR}/.config $f" || (exit 0)
        DEPENDS
            ${prev_target}
    )

    set(prev_target kernel_config_${filename})

endforeach()


# Do something with the old config?
add_custom_target(kernel_oldconfig
    WORKING_DIRECTORY
        ${KERNEL_SOURCE_DIR}
    COMMAND
        ${CMAKE_MAKE_PROGRAM} ARCH=arm oldconfig O="${KERNEL_BUILD_DIR}" -j${num_jobs}
    DEPENDS
        ${prev_target}
)

# Build kernel images (uImage and vmlinux) and device tree (dtbs)
add_custom_target(kernel_images_dtbs
    WORKING_DIRECTORY
        ${KERNEL_SOURCE_DIR}
    COMMAND
        ${CMAKE_MAKE_PROGRAM} ARCH=arm uImage vmlinux dtbs LOADADDR=0xC2000040 O="${KERNEL_BUILD_DIR}" -j${num_jobs}
    DEPENDS
        kernel_oldconfig
)

# Build kernel modules
add_custom_target(kernel_build_modules
    WORKING_DIRECTORY
        ${KERNEL_SOURCE_DIR}
    COMMAND
        ${CMAKE_MAKE_PROGRAM} ARCH=arm all O="${KERNEL_BUILD_DIR}" -j${num_jobs}
    DEPENDS
        kernel_images_dtbs
)

# Generate userspace headers and kernel modules
add_custom_target(kernel_install
    WORKING_DIRECTORY
        ${KERNEL_SOURCE_DIR}
    COMMAND
        ${CMAKE_MAKE_PROGRAM} ARCH=arm INSTALL_HDR_PATH="${KERNEL_BUILD_DIR}/install_artifact" headers_install O="${KERNEL_BUILD_DIR}" -j${num_jobs}
    COMMAND
        ${CMAKE_MAKE_PROGRAM} ARCH=arm INSTALL_MOD_PATH="${KERNEL_BUILD_DIR}/install_artifact" modules_install O="${KERNEL_BUILD_DIR}" -j${num_jobs}
    DEPENDS
        kernel_build_modules
)
