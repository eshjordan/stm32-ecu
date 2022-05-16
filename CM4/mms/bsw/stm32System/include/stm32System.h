#ifndef _STM32_H
#define _STM32_H

#include "main.h"
#include "openamp_log.h"
#include "linuxComms.h"
#include "stm32OpenampWrapper.h"

#define ALIGN __attribute__((aligned(4)))

#ifdef __cplusplus
extern "C" {
#endif

extern void initialise_monitor_handles();

void system_run(void);

int rand(void);

#ifdef __cplusplus
}
#endif

#endif
