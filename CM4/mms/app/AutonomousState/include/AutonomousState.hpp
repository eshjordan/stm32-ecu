
#include "AutonomousStateTypes.h"

class AutonomousState {

private:
  static AutonomousState_en currentState;

public:
  static bool requestState(AutonomousState_en state);
  static AutonomousState_en getCurrentState();
};
