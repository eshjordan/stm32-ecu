#include "System.hpp"

#include "stm32mp15xx_disco.h"
#include "stm32.h"


REGISTER_ROUTINE(point_mass_model, 10)
{
    // read parameters by name
    auto pos = System::get_parameter<double>("position");
    auto vel = System::get_parameter<double>("velocity");
    auto acc = System::get_parameter<double>("acceleration");

    // perform a calculation with the parameters
    pos = pos + vel * (1 / 10.0);
    vel = vel + acc * (1 / 10.0);

    // update parameter 2 by name
    System::set_parameter("position", pos);
    System::set_parameter("velocity", vel);
}


REGISTER_ROUTINE(print_state, 1)
{
    auto pos = System::get_parameter<double>("position");
    auto vel = System::get_parameter<double>("velocity");
    auto acc = System::get_parameter<double>("acceleration");

    BSP_LED_Toggle(LED_GREEN);

    printf("pos: %lf, vel: %lf, acc: %lf\n", pos, vel, acc);
    printf("Analogue Input 4: %lumV\n", System::IO::read_analogue_input(4));
    log_info("print_state_run");
}
