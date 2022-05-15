#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

unset LD_LIBRARY_PATH
source ${HOME}/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
cd ${SCRIPT_DIR}/gdbscripts
${GDB} -x Setup.gdb
