#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <linux/stm32ecu/shared/Interproc_Msg.h>
#include <linux/stm32ecu/shared/Parameter.h>
#include <linux/stm32ecu/stm32ecu.h>

#include "ecu.grpc.pb.h"
#include "ecu.pb.h"
#include "grpcpp/server_builder.h"

static int fd = -1;

class RouteGuideImpl final : public ecu_grpc::EcuService::Service {
	::grpc::Status reset(::grpc::ServerContext *context,
			     const ::ecu_grpc::Empty *request,
			     ::ecu_grpc::Empty *response) override
	{
		int status = ioctl(fd, STM32ECU_RESET);
		if (status == -1) {
			printf("Couldn't ioctl MY_RESET!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}
		printf("MY_RESET: OK!\n");
		return ::grpc::Status::OK;
	}

	::grpc::Status offline(::grpc::ServerContext *context,
			       const ::ecu_grpc::Empty *request,
			       ::ecu_grpc::Empty *response) override
	{
		int status = ioctl(fd, STM32ECU_OFFLINE);
		if (status == -1) {
			printf("Couldn't ioctl MY_OFFLINE!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}
		printf("MY_OFFLINE: OK!\n");
		return ::grpc::Status::OK;
	}

	::grpc::Status getState(::grpc::ServerContext *context,
				const ::ecu_grpc::Empty *request,
				::ecu_grpc::State *response) override
	{
		char rcv_msg[36] = { 0 };

		int status = ioctl(fd, STM32ECU_GET_STATE, rcv_msg);
		if (status == -1) {
			printf("Couldn't ioctl MY_GETSTATE!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}
		printf("MY_GETSTATE: %s - OK!\n", rcv_msg);
		return ::grpc::Status::OK;
	}

	::grpc::Status setState(::grpc::ServerContext *context,
				const ::ecu_grpc::State *request,
				::ecu_grpc::Empty *response) override
	{
		static const char usr_msg[] =
			"ioctl message, hey from user space!";

		int status = ioctl(fd, STM32ECU_SET_STATE, usr_msg);
		if (status == -1) {
			printf("Couldn't ioctl MY_SETSTATE!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}
		printf("MY_SETSTATE: OK!\n");
		return ::grpc::Status::OK;
	}

	::grpc::Status ping(::grpc::ServerContext *context,
			    const ::ecu_grpc::Empty *request,
			    ::ecu_grpc::Empty *response) override
	{
		int status = ioctl(fd, STM32ECU_PING_RPROC);
		if (status == -1) {
			printf("Couldn't ioctl MY_PING_RPROC!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}
		printf("MY_PING_RPROC: OK!\n");
		return ::grpc::Status::OK;
	}

	::grpc::Status sendMsg(::grpc::ServerContext *context,
			       const ::ecu_grpc::InterprocMsg *request,
			       ::ecu_grpc::InterprocMsg *response) override
	{
		uint8_t data[16] = {0};
		data[0] = Parameter_Type_t::PARAMETER_DOUBLE;
		strncpy((char*)&data[1], "position", 9);
		Interproc_Msg_t msg = interproc_msg_make(Interproc_Command_t::CMD_PARAM_GET, data, strlen("position")+1);
		int status = ioctl(fd, STM32ECU_SEND_MSG, &msg);
		if (status == -EINVAL) {
			printf("Couldn't ioctl MY_SEND_MSG_RPROC!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		} else if (status < 0) {
			printf("Some other error, couldn't ioctl MY_SEND_MSG_RPROC!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}

		printf("MY_SEND_MSG_RPROC: OK!\n");

		uint32_t response_id = *(uint32_t*)&msg;

		uint8_t loop_count = 0;
		while (-EAGAIN == (status = ioctl(fd, STM32ECU_RECV_MSG, &msg)) && loop_count++ < 10) {
			sleep(1);
		}

		if (status == -EINVAL) {
			printf("Couldn't ioctl MY_RECV_MSG_RPROC!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		} else if (status == -EAGAIN) {
			printf("Couldn't ioctl MY_RECV_MSG_RPROC, timeout!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		} else if (status < 0) {
			printf("Some other error, couldn't ioctl MY_RECV_MSG_RPROC!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}

		response->set_command((::ecu_grpc::InterprocMsg_InterprocCommand)msg.command);
		response->set_data(msg.data, sizeof(msg.data));
		response->set_checksum(msg.checksum);

		printf("MY_RECV_MSG_RPROC: OK!\n");
		return ::grpc::Status::OK;
	}
};

void RunServer()
{
	std::string server_address("0.0.0.0:50051");
	RouteGuideImpl service;

	grpc::ServerBuilder builder;
	builder.AddListeningPort(server_address,
				 grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;
	server->Wait();
}

void sig_handler(int signum)
{
	//Return type of the handler function should be void
	printf("\nInside handler function\n");
	close(fd);
	switch (signum) {
	case SIGINT: {
		exit(0);
	}
	default: {
		exit(1);
	}
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGSTOP, sig_handler);

	int status;

	printf("Hello OpenSTLinux World!\n");

	fd = open("/dev/stm32ecu", O_RDWR);

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


	uint8_t data[16] = {0};
	data[0] = Parameter_Type_t::PARAMETER_DOUBLE;
	strncpy((char*)&data[1], "position", 9);

	Interproc_Msg_t tx_msg = interproc_msg_make(Interproc_Command_t::CMD_PARAM_GET, data, strlen("position")+1);
	auto bytes_written = write(fd, &tx_msg, sizeof(tx_msg));

	if (bytes_written != sizeof(tx_msg)) {
		printf("Couldn't write to '/dev/stm32ecu'!\n");
		close(fd);
		return 1;
	}

	printf("WRITE MESSAGE: OK!\n");

	Interproc_Msg_t rx_msg = {0};
	ssize_t bytes_read = 0;
	uint8_t loop_count = 0;
	while (0 == (bytes_read = read(fd, &rx_msg, sizeof(rx_msg))) && loop_count++ < 10) {
		sleep(1);
	}

	if (bytes_read != sizeof(rx_msg)) {
		printf("Couldn't read from '/dev/stm32ecu' (%d bytes) !\n", bytes_read);
		close(fd);
		return 1;
	}

	if (rx_msg.checksum != interproc_msg_calc_checksum(&rx_msg)) {
		printf("rx_msg checksum mismatch!\n");
		close(fd);
		return 1;
	}

	if (rx_msg.command != Interproc_Command_t::CMD_ACK) {
		printf("Unexpected rx_msg type - %d\n", rx_msg.command);
		close(fd);
		return 1;
	}

	printf("rx_msg.data - position: %lf\n", *(double*)rx_msg.data);

	printf("READ MESSAGE: OK!\n");


	// uint8_t data[16] = {0};
	// data[0] = Parameter_Type_t::PARAMETER_DOUBLE;
	// strncpy((char*)&data[1], "position", 9);
	// Interproc_Msg_t msg = interproc_msg_make(Interproc_Command_t::CMD_PARAM_GET, data, strlen("position")+1);
	// status = ioctl(fd, STM32ECU_SEND_MSG, &msg);
	// if (status == -EINVAL) {
	// 	printf("Couldn't ioctl MY_SEND_MSG_RPROC!\n");
	// 	close(fd);
	// 	return 1;
	// } else if (status < 0) {
	// 	printf("Some other error, couldn't ioctl MY_SEND_MSG_RPROC!\n");
	// 	close(fd);
	// 	return 1;
	// }

	// printf("MY_SEND_MSG_RPROC: OK!\n");

	// uint32_t response_id = *(uint32_t*)&msg;

	// uint8_t loop_count = 0;
	// while (-EAGAIN == (status = ioctl(fd, STM32ECU_RECV_MSG, &msg)) && loop_count++ < 10) {
	// 	sleep(1);
	// }

	// if (status == -EINVAL) {
	// 	printf("Couldn't ioctl MY_RECV_MSG_RPROC!\n");
	// 	close(fd);
	// 	return 1;
	// } else if (status == -EAGAIN) {
	// 	printf("Couldn't ioctl MY_RECV_MSG_RPROC, timeout!\n");
	// 	close(fd);
	// 	return 1;
	// } else if (status < 0) {
	// 	printf("Some other error, couldn't ioctl MY_RECV_MSG_RPROC!\n");
	// 	close(fd);
	// 	return 1;
	// }

	// printf("MY_RECV_MSG_RPROC: OK!\n");

	printf("stm32ecu ioctl success!\n");

	RunServer();

	close(fd);

	return 0;
}
