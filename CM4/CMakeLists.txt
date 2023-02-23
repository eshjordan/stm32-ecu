
cmake_minimum_required(VERSION 3.0)

project(stm32-ecu_CM4 LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE EngDebug)
endif()


function(RemoveFlag VAR SCOPE FLAG DOCSTRING)
	if(NOT "${${VAR}}" STREQUAL "")
		MESSAGE("Removing ${FLAG} flag from ${VAR}")
		separate_arguments(${VAR})
		list(REMOVE_ITEM ${VAR} ${FLAG})
		string(REPLACE ";" " " ${VAR} "${${VAR}}")

		if("${SCOPE}" STREQUAL "CACHE")
			set(${VAR} "${${VAR}}" CACHE STRING "${DOCSTRING}" FORCE)
		elseif("${SCOPE}" STREQUAL "SCRIPT")
			set(${VAR} "${${VAR}}" PARENT_SCOPE)
		endif()
	endif()
endfunction()


set(CMAKE_EXECUTABLE_SUFFIX ".elf")


set(ENG_OPTIONS
    ${GENERAL_OPTIONS}
)

set(PROD_OPTIONS
    ${GENERAL_OPTIONS}
    # "-D__LOG_TRACE_IO_"
)

if (CMAKE_BUILD_TYPE STREQUAL EngDebug)

    add_compile_options(
        ${ENG_OPTIONS}
        ${DEBUG_OPTIONS}
        ${LINKER_OPTIONS}
    )

elseif(CMAKE_BUILD_TYPE STREQUAL EngRelease)

    add_compile_options(
        ${ENG_OPTIONS}
        ${RELEASE_OPTIONS}
        ${LINKER_OPTIONS}
    )

elseif(CMAKE_BUILD_TYPE STREQUAL ProdDebug)

    add_compile_options(
        ${PROD_OPTIONS}
        ${DEBUG_OPTIONS}
        ${LINKER_OPTIONS}
    )

elseif(CMAKE_BUILD_TYPE STREQUAL ProdRelease)

    add_compile_options(
        ${PROD_OPTIONS}
        ${RELEASE_OPTIONS}
        ${LINKER_OPTIONS}
    )

endif()





include_directories(
    # "-I/opt/st/stm32cubeide_1.8.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.9-2020-q2-update.linux64_2.0.0.202105311346/tools/arm-none-eabi/include"
    "${CMAKE_CURRENT_LIST_DIR}/Core"
    "${CMAKE_CURRENT_LIST_DIR}/Core/Inc"
    "${CMAKE_CURRENT_LIST_DIR}/Drivers/BSP/STM32MP15xx_DISCO"
    "${CMAKE_CURRENT_LIST_DIR}/OPENAMP"
    "${CMAKE_CURRENT_LIST_DIR}/ecu-dev/src/libs"
    "${CMAKE_CURRENT_LIST_DIR}/ecu-dev/src/shared"
    "${CMAKE_CURRENT_LIST_DIR}/ecu-dev/src/shared/include"
    "${CMAKE_CURRENT_LIST_DIR}/ecu-dev/src/sdk"
    "${CMAKE_CURRENT_LIST_DIR}/ecu-dev/src/sdk/include"
    "${CMAKE_CURRENT_LIST_DIR}/../Drivers/CMSIS/Device/ST/STM32MP1xx/Include"
    "${CMAKE_CURRENT_LIST_DIR}/../Drivers/CMSIS/Include"
    "${CMAKE_CURRENT_LIST_DIR}/../Drivers/STM32MP1xx_HAL_Driver/Inc"
    "${CMAKE_CURRENT_LIST_DIR}/../Drivers/STM32MP1xx_HAL_Driver/Inc/Legacy"
    "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/include"
    "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS"
    "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F"
    "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/OpenAMP/open-amp/lib/include"
    "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/OpenAMP/libmetal/lib/include"
    "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/OpenAMP/virtual_driver"
)



file(GLOB_RECURSE CORE_SRC
    Core/*.c
    Core/*.cpp
)

file(GLOB_RECURSE BSP_SRC
    Drivers/BSP/*.c
    Drivers/BSP/*.cpp
    Drivers/BSP/*.s
)

file(GLOB_RECURSE OPENAMP_SRC
    OPENAMP/*.c
    OPENAMP/*.cpp
    OPENAMP/*.s
)

set(STMPROJ_PREFIX "${CMAKE_CURRENT_LIST_DIR}/..")

file(GLOB_RECURSE COMMON_SRC
    ${STMPROJ_PREFIX}/Common/*.c
    ${STMPROJ_PREFIX}/Common/*.cpp
    ${STMPROJ_PREFIX}/Common/*.s
)

file(GLOB_RECURSE DRIVERS_SRC
    ${STMPROJ_PREFIX}/Drivers/*.c
    ${STMPROJ_PREFIX}/Drivers/*.cpp
    ${STMPROJ_PREFIX}/Drivers/*.s
)

file(GLOB_RECURSE MIDDLEWARES_SRC
    ${STMPROJ_PREFIX}/Middlewares/*.c
    ${STMPROJ_PREFIX}/Middlewares/*.cpp
    ${STMPROJ_PREFIX}/Middlewares/*.s
)

file(GLOB ECU_DEV
    ecu-dev/src/libs/RTOS.cpp
    ecu-dev/src/sdk/*.c
    ecu-dev/src/sdk/*.cpp
    ecu-dev/src/sdk/portable/stm32mp1/*.c
    ecu-dev/src/sdk/portable/stm32mp1/*.cpp
    ecu-dev/src/shared/*.c
    ecu-dev/src/shared/*.cpp
)

add_library(${PROJECT_NAME}_obj OBJECT
    ${CORE_SRC}
    ${BSP_SRC}
    ${OPENAMP_SRC}
    ${COMMON_SRC}
    ${DRIVERS_SRC}
    ${MIDDLEWARES_SRC}
    ${ECU_DEV}
)


set(COMPILE_OPTS "SHELL:-c -mcpu=cortex-m4 -mlittle-endian -fsso-struct=little-endian -std=c99 -std=c++11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32MP157Dxx -DSTM32_BUILD -DNO_ATOMIC_64_SUPPORT -DMETAL_INTERNAL -DMETAL_MAX_DEVICE_REGIONS=2 -DVIRTIO_SLAVE_ONLY -DMETAL_FREERTOS -DUSERSPACE_BUILD -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")
set(STARTUP_COMPILE_OPTS "${COMPILE_OPTS} -x assembler-with-cpp -MMD -MP")

target_compile_options(${PROJECT_NAME}_obj PUBLIC
    ${COMPILE_OPTS}
)

add_library(${PROJECT_NAME}_start_obj OBJECT)

target_sources(${PROJECT_NAME}_start_obj PRIVATE
    Core/Startup/startup_stm32mp157dacx.s
)

target_compile_options(${PROJECT_NAME}_start_obj PUBLIC
    ${STARTUP_COMPILE_OPTS}
)


RemoveFlag(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS SCRIPT "-rdynamic" "")
RemoveFlag(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS SCRIPT "-rdynamic" "")


add_executable(${PROJECT_NAME})

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32MP157DACX_RAM.ld")

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_NAME}_obj
    ${PROJECT_NAME}_start_obj
)


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs --specs=nano.specs -Wl,-T${LINKER_SCRIPT} -mlittle-endian -fsso-struct=little-endian -mcpu=cortex-m4 -Wl,-Map=\"stm32-ecu_CM4.map\" -Wl,--gc-sections -static -lc -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--no-wchar-size-warning -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group")


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} ./${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ./${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} -O binary ./${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJDUMP} -l -h -S -D ./${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} > ./${PROJECT_NAME}.list
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Convert the ELF output file to a binary image" )

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

unset(CMAKE_BUILD_TYPE CACHE)
