#include "System.hpp"

#include "AutonomousState.hpp"

#include "stm32mp15xx_disco.h"

#include "stm32System.h"


REGISTER_ROUTINE(point_mass_model, 10, 128)
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

REGISTER_ROUTINE(print_state, 1, 512)
{
	auto pos = System::get_parameter<double>("position");
	auto vel = System::get_parameter<double>("velocity");
	auto acc = System::get_parameter<double>("acceleration");

	if (AS_STATE_EMERGENCY != AutonomousState::getCurrentState()) {
		AutonomousState::requestState(AS_STATE_EMERGENCY);
	} else {
		AutonomousState::requestState(AS_STATE_OFF);
	}

	CAN_Msg_t msg;
	msg.header = header_make(0, sizeof(CAN_Msg_t));
	msg.identifier = 0x123;
	msg.data_length_code = 3;
	msg.data[0] = 1;
	msg.data[1] = 2;
	msg.data[2] = 3;
	can_msg_calc_checksum(&msg);
	System::IO::write_can_output(IO_CAN_CHANNEL_01, 0x700, msg);

	CAN_Msg_t msg_02 = System::IO::read_can_input(IO_CAN_CHANNEL_01, 0x700);

	printf("pos: %lf, vel: %lf, acc: %lf\n", pos, vel, acc);
	uint32_t analogue_ch_05 = System::IO::read_analogue_input(IO_ADC_CHANNEL_05);
	printf("Analogue Input 5: %lumV\n", analogue_ch_05);
	log_info("print_state_run");
}
