#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/stm32ecu/stm32ecu.h>
#include <linux/stm32ecu/Interproc_Msg.h>

int main(int argc, char **argv)
{
	int status;

	printf("Hello OpenSTLinux World!\n");

	int fd = open("/dev/stm32ecu", O_RDWR);

	if (fd == -1) {
		printf("Couldn't open '/dev/stm32ecu'!\n");
		return 1;
	}

	status = ioctl(fd, STM32ECU_RESET);
	if (status == -1) {
		printf("Couldn't ioctl MY_RESET!\n");
		close(fd);
		return 1;
	}
	printf("MY_RESET: OK!\n");

	status = ioctl(fd, STM32ECU_OFFLINE);
	if (status == -1) {
		printf("Couldn't ioctl MY_OFFLINE!\n");
		close(fd);
		return 1;
	}
	printf("MY_OFFLINE: OK!\n");

	char rcv_msg[36] = { 0 };

	status = ioctl(fd, STM32ECU_GET_STATE, rcv_msg);
	if (status == -1) {
		printf("Couldn't ioctl MY_GETSTATE!\n");
		close(fd);
		return 1;
	}
	printf("MY_GETSTATE: %s - OK!\n", rcv_msg);

	static const char usr_msg[] = "ioctl message, hey from user space!";

	status = ioctl(fd, STM32ECU_SET_STATE, usr_msg);
	if (status == -1) {
		printf("Couldn't ioctl MY_SETSTATE!\n");
		close(fd);
		return 1;
	}
	printf("MY_SETSTATE: OK!\n");

	status = ioctl(fd, STM32ECU_PING_RPROC);
	if (status == -1) {
		printf("Couldn't ioctl MY_PING_RPROC!\n");
		close(fd);
		return 1;
	}
	printf("MY_PING_RPROC: OK!\n");

	Interproc_Msg_t msg = interproc_msg_make(Interproc_Command_t::RESET_CMD, nullptr);
	status = ioctl(fd, STM32ECU_SEND_MSG, &msg);
	if (status == -1) {
		printf("Couldn't ioctl MY_SEND_MSG_RPROC!\n");
		close(fd);
		return 1;
	}
	printf("MY_SEND_MSG_RPROC: OK!\n");

	printf("stm32ecu ioctl success!\n");

	close(fd);

	return 0;
}
