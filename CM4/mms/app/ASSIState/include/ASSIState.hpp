
#pragma once

#include "ASSIStateTypes.h"

class ASSIState {

private:
  static ASSIState_en currentState;

public:
  static bool requestState(ASSIState_en state);
  static ASSIState_en getCurrentState();
};
