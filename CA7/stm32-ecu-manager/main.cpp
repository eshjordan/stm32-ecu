#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/stm32ecu/stm32ecu.h>

int main(int argc, char **argv)
{
	printf("Hello OpenSTLinux World!\n");

	int fd = open("/dev/stm32ecu", O_RDWR);

	if (fd == -1) {
		printf("Couldn't open '/dev/stm32ecu'!\n");
		return 1;
	}

	int status = ioctl(fd, STM32ECU_RESET);
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

	printf("stm32ecu ioctl success!\n");

	close(fd);

	return 0;
}
