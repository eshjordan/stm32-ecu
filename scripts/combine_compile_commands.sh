#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
${SCRIPT_DIR}/../CA7/linux-5.10.61/linux-5.10.61/scripts/clang-tools/gen_compile_commands.py \
    ${SCRIPT_DIR}/../CA7/linux-5.10.61/build \
    -d ${SCRIPT_DIR}/.. \
    -o ${SCRIPT_DIR}/../CA7/linux-5.10.61/compile_commands.json

python3 ${SCRIPT_DIR}/combine_compile_commands.py
