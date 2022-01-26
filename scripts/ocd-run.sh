#!/bin/bash
unset LD_LIBRARY_PATH
source /opt/st/stm32mp1/3.1-openstlinux-5.10-dunfell-mp1-21-03-31/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
openocd -f /opt/st/stm32mp1/3.1-openstlinux-5.10-dunfell-mp1-21-03-31/sysroots/x86_64-ostl_sdk-linux/usr/share/openocd/scripts/board/stm32mp15x_dk2.cfg
