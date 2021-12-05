#include "stm32.h"
#include "main.h"
#include "System.hpp"

static RNG_HandleTypeDef hrng = {};
extern osTimerId esp_in_update_tmrHandle;
extern osTimerId esp_out_update_tmrHandle;

void system_init(int argc, char *argv[])
{
	initialise_monitor_handles();
	HAL_RNG_MspInit(&hrng);
	if(HAL_RNG_Init(&hrng) != HAL_OK)
	{
		Error_Handler();
	}

	if(osTimerStart(esp_in_update_tmrHandle, 100) != osOK)
	{
	 Error_Handler();
	}

	if(osTimerStart(esp_out_update_tmrHandle, 100) != osOK)
	{
	  Error_Handler();
	}

	System::init(argc, argv);
}

void system_run(void)
{
	system_init(0, NULL);
	System::run();
}

uint32_t rand(void)
{
	uint32_t rnd = 0;
	HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
	return rnd;
}
