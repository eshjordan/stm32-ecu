#include "Interproc_Msg.h"
#include "cmsis_os.h"
#include "stm32mp15xx_disco.h"
#include "stm32mp1xx_hal.h"

#include "System.hpp"
#include "stm32System.h"

static RNG_HandleTypeDef hrng = {};
extern CRC_HandleTypeDef hcrc2;
extern osTimerId esp_in_update_tmrHandle;
extern osTimerId esp_out_update_tmrHandle;

extern FDCAN_HandleTypeDef hfdcan1;
extern SPI_HandleTypeDef hspi4;
extern SPI_HandleTypeDef hspi5;

FDCAN_HandleTypeDef *const can_bus_01_handle = &hfdcan1;
SPI_HandleTypeDef *const mcp2515_spi_handle = &hspi4;
SPI_HandleTypeDef *const esp32_spi_handle = &hspi5;

static void stm32StartTimers(void) {
	if(osTimerStart(esp_in_update_tmrHandle, 1000) != osOK)
	{
	 Error_Handler();
	}

	if(osTimerStart(esp_out_update_tmrHandle, 1000) != osOK)
	{
	  Error_Handler();
	}
}

void system_run(void)
{
	if(IS_ENGINEERING_BOOT_MODE())
	{
		initialise_monitor_handles();
	}


	hrng.Instance = RNG1;
	hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
	HAL_RNG_MspInit(&hrng);
	if(HAL_RNG_Init(&hrng) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_FDCAN_MspInit(can_bus_01_handle);
	if(HAL_FDCAN_Init(can_bus_01_handle) != HAL_OK)
	{
		Error_Handler();
	}

	if(HAL_FDCAN_Start(can_bus_01_handle) != HAL_OK)
	{
		Error_Handler();
	}

	stm32StartTimers();

	BSP_LED_Init(LED_GREEN);
	BSP_LED_On(LED_GREEN);

	System::init(0, nullptr);
	System::run();
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
