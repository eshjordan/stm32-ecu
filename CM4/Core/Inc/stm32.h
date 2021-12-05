#ifndef _STM32_H
#define _STM32_H

#include "main.h"
#include "cmsis_os.h"
#include "stm32mp1xx_hal.h"
//#include "stm32mp15xx_disco.h"

#define ALIGN __attribute__((aligned(4)))

#ifdef __cplusplus
extern "C" {
#endif

extern void initialise_monitor_handles();

void system_init(int argc, char *argv[]);
void system_run(void);

uint32_t rand(void);

#ifdef __cplusplus
}
#endif

#endif
