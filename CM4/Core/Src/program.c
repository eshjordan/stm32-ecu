#include "program.h"
#include "System.h"

#include "stm32.h"
#include "stm32mp15xx_disco.h"

void register_routines() {
  REGISTER_ROUTINE(point_mass_model, 10, 128);
  REGISTER_ROUTINE(print_state, 1, 512);
}

DEFINE_ROUTINE(point_mass_model, 10, 128) {
  // read parameters by name
  double pos;
  double vel;
  double acc;

  systemGetParameter("position", &pos, NULL);
  systemGetParameter("velocity", &vel, NULL);
  systemGetParameter("acceleration", &acc, NULL);

  // perform a calculation with the parameters
  pos = pos + vel * (1 / 10.0);
  vel = vel + acc * (1 / 10.0);

  // update parameter 2 by name
  systemSetParameter("position", &pos, PARAM_DOUBLE, sizeof(double));
  systemSetParameter("velocity", &vel, PARAM_DOUBLE, sizeof(double));
}

DEFINE_ROUTINE(print_state, 1, 512) {
  double pos;
  double vel;
  double acc;

  systemGetParameter("position", &pos, NULL);
  systemGetParameter("velocity", &vel, NULL);
  systemGetParameter("acceleration", &acc, NULL);

  BSP_LED_Toggle(LED_GREEN);

  printf("pos: %lf, vel: %lf, acc: %lf\n", pos, vel, acc);
  printf("Analogue Input 4: %lumV\n", read_analogue_input(4));
  log_info("print_state_run");
}
