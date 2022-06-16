#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <linux/stm32ecu/Interproc_Msg.h>
#include <linux/stm32ecu/stm32ecu.h>

#include "ecu.grpc.pb.h"
#include "ecu.pb.h"
#include "grpcpp/server_builder.h"

static int fd = -1;

class RouteGuideImpl final : public ecu_grpc::EcuService::Service {
	::grpc::Status reset(::grpc::ServerContext *context,
			     const ::ecu_grpc::Empty *request,
			     ::ecu_grpc::State *response) override
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
			       ::ecu_grpc::State *response) override
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
				::ecu_grpc::State *response) override
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
			    ::ecu_grpc::State *response) override
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
			       ::ecu_grpc::State *response) override
	{
		Interproc_Msg_t msg = interproc_msg_make(
			Interproc_Command_t::RESET_CMD, nullptr);
		int status = ioctl(fd, STM32ECU_SEND_MSG, &msg);
		if (status == -1) {
			printf("Couldn't ioctl MY_SEND_MSG_RPROC!\n");
			close(fd);
			return ::grpc::Status::CANCELLED;
		}
		printf("MY_SEND_MSG_RPROC: OK!\n");
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

	Interproc_Msg_t msg =
		interproc_msg_make(Interproc_Command_t::RESET_CMD, nullptr);
	status = ioctl(fd, STM32ECU_SEND_MSG, &msg);
	if (status == -1) {
		printf("Couldn't ioctl MY_SEND_MSG_RPROC!\n");
		close(fd);
		return 1;
	}
	printf("MY_SEND_MSG_RPROC: OK!\n");

	printf("stm32ecu ioctl success!\n");

	RunServer();

	close(fd);

	return 0;
}
