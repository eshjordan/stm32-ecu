#include "stm32.h"
#include "Interproc_Msg.h"
#include "System.h"
#include "cmsis_os.h"
#include "stm32mp15xx_disco.h"

#include "System.h"

static RNG_HandleTypeDef hrng = {0};
extern CRC_HandleTypeDef hcrc2;
extern osTimerId esp_in_update_tmrHandle;
extern osTimerId esp_out_update_tmrHandle;

void system_run(void)
{
	if(IS_ENGINEERING_BOOT_MODE())
	{
		initialise_monitor_handles();
	}

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

	BSP_LED_Init(LED_GREEN);
	BSP_LED_On(LED_GREEN);

	systemInit(0, NULL);
	systemRun();
}

int rand(void)
{
	uint32_t rnd = 0;
	HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
	return rnd;
}


//int _write(int file, char *ptr, int len)
//{
//	int DataIdx;
//
//	for (DataIdx = 0; DataIdx < len; DataIdx++)
//	{
//	   ITM_SendChar( *ptr++ );
//	}
//
//	return len;
//}
