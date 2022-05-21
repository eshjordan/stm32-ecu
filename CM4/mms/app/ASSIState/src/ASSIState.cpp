
#include "ASSIState.hpp"
#include "stm32mp15xx_disco.h"

ASSIState_en ASSIState::currentState = ASSI_STATE_OFF;

bool ASSIState::requestState(ASSIState_en state) {
  bool conditionOk = false;

  switch (state) {
  case ASSI_STATE_OFF: {
    int32_t status = BSP_LED_Off(LED_GREEN);
    conditionOk = BSP_ERROR_NONE == status;
    break;
  }
  case ASSI_STATE_YELLOW_SOLID: {
    int32_t status = BSP_LED_Off(LED_GREEN);
    conditionOk = BSP_ERROR_NONE == status;
    break;
  }
  case ASSI_STATE_YELLOW_FLASH: {
    int32_t status = BSP_LED_Off(LED_GREEN);
    conditionOk = BSP_ERROR_NONE == status;
    break;
  }
  case ASSI_STATE_BLUE_FLASH: {
    int32_t status = BSP_LED_On(LED_GREEN);
    conditionOk = BSP_ERROR_NONE == status;
    break;
  }
  case ASSI_STATE_BLUE_SOLID: {
    int32_t status = BSP_LED_Off(LED_GREEN);
    conditionOk = BSP_ERROR_NONE == status;
    break;
  }
  default: {
    conditionOk = false;
    break;
  }
  }

  if (conditionOk) {
    currentState = state;
    return true;
  }

  return false;
}

ASSIState_en ASSIState::getCurrentState() { return currentState; }
