#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "Interproc_Msg.h"
#include "cmsis_os.h"
#include "main.h"

void runLinuxComms(void const *argument);

#ifdef __cplusplus
}
#endif
