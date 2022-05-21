
#include "AutonomousState.hpp"
#include "ASSIState.hpp"

AutonomousState_en AutonomousState::currentState = AS_STATE_OFF;

bool AutonomousState::requestState(AutonomousState_en state) {
  bool assiChangeStateOk = false;

  switch (state) {
  case AS_STATE_OFF: {
    assiChangeStateOk = ASSIState::requestState(ASSI_STATE_OFF);
    break;
  }
  case AS_STATE_READY: {
    assiChangeStateOk = ASSIState::requestState(ASSI_STATE_YELLOW_SOLID);
    break;
  }
  case AS_STATE_DRIVING: {
    assiChangeStateOk = ASSIState::requestState(ASSI_STATE_YELLOW_FLASH);
    break;
  }
  case AS_STATE_EMERGENCY: {
    assiChangeStateOk = ASSIState::requestState(ASSI_STATE_BLUE_FLASH);
    break;
  }
  case AS_STATE_FINISHED: {
    assiChangeStateOk = ASSIState::requestState(ASSI_STATE_BLUE_SOLID);
    break;
  }
  default: {
    assiChangeStateOk = false;
    break;
  }
  }

  if (assiChangeStateOk) {
    currentState = state;
    return true;
  }

  return false;
}

AutonomousState_en AutonomousState::getCurrentState() { return currentState; }
