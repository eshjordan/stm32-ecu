#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

unset LD_LIBRARY_PATH
source /opt/st/stm32mp1/3.1-openstlinux-5.10-dunfell-mp1-21-03-31/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
cd ${SCRIPT_DIR}/gdbscripts
${GDB} -x Setup.gdb
