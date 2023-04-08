#include "System.hpp"

#include "AutonomousState.hpp"

#include "openamp_log.h"
#include "stm32mp15xx_disco.h"

#include "stm32System.h"


INIT_MODULE(Accumulator)
{
	System::add_parameter<double>("acceleration", 1.0f);
	System::add_channel<double>("position", 0.0f, ChannelLogRate::CHANNEL_LOG_100HZ);
	System::add_channel<double>("velocity", 0.0f, ChannelLogRate::CHANNEL_LOG_100HZ);
}

REGISTER_ROUTINE(point_mass_model, 10, 256)
{
	// read parameters by name
	auto acc = System::get_parameter_value<double>("acceleration");
	auto vel = System::get_channel_value<double>("velocity");
	auto pos = System::get_channel_value<double>("position");

	// perform a calculation with the parameters
	pos = pos + vel * (1 / 10.0);
	vel = vel + acc * (1 / 10.0);

	// update parameter 2 by name
	System::set_channel_value("velocity", vel);
	System::set_channel_value("position", pos);
}

REGISTER_ROUTINE(print_state, 1, 1024)
{
	auto acc = System::get_parameter_value<double>("acceleration");
	auto vel = System::get_channel_value<double>("velocity");
	auto pos = System::get_channel_value<double>("position");

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
