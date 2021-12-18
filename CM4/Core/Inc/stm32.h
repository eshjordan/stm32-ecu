#ifndef _STM32_H
#define _STM32_H

#include "main.h"

#define ALIGN __attribute__((aligned(4)))

#ifdef __cplusplus
extern "C" {
#endif

extern void initialise_monitor_handles();

void system_run(void);

uint32_t rand(void);

#ifdef __cplusplus
}
#endif

#endif
