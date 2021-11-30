#include "stm32.h"
#include "System.hpp"

void system_init(int argc, char *argv[])
{
	System::init(argc, argv);
}

void system_run(void)
{
	System::run();
}
