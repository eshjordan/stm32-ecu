#include "System.hpp"

#include "AutonomousState.hpp"

#include "openamp_log.h"
#include "stm32mp15xx_disco.h"

#include "stm32System.h"


INIT_MODULE(Accumulator)
{
	System::add_parameter<double>("position", 0.0f);
	System::add_parameter<double>("velocity", 0.0f);
	System::add_parameter<double>("acceleration", 1.0f);
//	System::add_channel<double>("accel_log", 0.0f, CHANNEL_LOG_100HZ);
}

REGISTER_ROUTINE(point_mass_model, 10, 256)
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

REGISTER_ROUTINE(print_state, 1, 256)
{
	auto pos = System::get_parameter<double>("position");
	auto vel = System::get_parameter<double>("velocity");
	auto acc = System::get_parameter<double>("acceleration");

	if (AS_STATE_EMERGENCY != AutonomousState::getCurrentState()) {
		AutonomousState::requestState(AS_STATE_EMERGENCY);
	} else {
		AutonomousState::requestState(AS_STATE_OFF);
	}

	log_info("pos: %lf, vel: %lf, acc: %lf\n", pos, vel, acc);
	uint32_t analogue_ch_05 = System::IO::read_analogue_input(IO_ADC_CHANNEL_05);
	log_info("Analogue Input 5: %lumV\n", analogue_ch_05);
	log_info("print_state_run");
}
