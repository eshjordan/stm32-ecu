#!/bin/bash
unset LD_LIBRARY_PATH
source ${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
openocd -f ${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/sysroots/x86_64-ostl_sdk-linux/usr/share/openocd/scripts/board/stm32mp15x_dk2.cfg
