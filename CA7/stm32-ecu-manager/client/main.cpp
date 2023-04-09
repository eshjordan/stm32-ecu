#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

#include "ecu.grpc.pb.h"
#include "ecu.pb.h"
#include "grpcpp/grpcpp.h"

class HelloClient {
    private:
	std::shared_ptr<grpc::Channel> channel_;
	std::unique_ptr<ecu_grpc::EcuService::Stub> stub_;

    public:
	HelloClient()
	{
		channel_ =
			grpc::CreateChannel("192.168.0.4:50051",
					    grpc::InsecureChannelCredentials());
		stub_ = ecu_grpc::EcuService::NewStub(channel_);
	}

	// Assembles the client's payload, sends it and presents the response back
	// from the server.
	void reset()
	{
		grpc::ClientContext context;
		ecu_grpc::Empty request;
		ecu_grpc::Empty reply;

		grpc::Status status = stub_->reset(&context, request, &reply);

		if (!status.ok()) {
			std::cout << status.error_code() << ": "
				  << status.error_message() << std::endl;
		}

		// return reply.message();
	}

	void offline()
	{
		grpc::ClientContext context;
		ecu_grpc::Empty request;
		ecu_grpc::Empty reply;

		grpc::Status status = stub_->offline(&context, request, &reply);
		if (!status.ok()) {
			std::cout << status.error_code() << ": "
				  << status.error_message() << std::endl;
		}
	}

	void getState()
	{
		grpc::ClientContext context;
		ecu_grpc::Empty request;
		ecu_grpc::State reply;

		grpc::Status status =
			stub_->getState(&context, request, &reply);
		if (!status.ok()) {
			std::cout << status.error_code() << ": "
				  << status.error_message() << std::endl;
		}
	}

	void setState()
	{
		grpc::ClientContext context;
		ecu_grpc::State request;
		ecu_grpc::Empty reply;

		grpc::Status status =
			stub_->setState(&context, request, &reply);
		if (!status.ok()) {
			std::cout << status.error_code() << ": "
				  << status.error_message() << std::endl;
		}
	}

	void ping()
	{
		grpc::ClientContext context;
		ecu_grpc::Empty request;
		ecu_grpc::Empty reply;

		grpc::Status status = stub_->ping(&context, request, &reply);
		if (!status.ok()) {
			std::cout << status.error_code() << ": "
				  << status.error_message() << std::endl;
		}
	}

	void sendMsg()
	{
		grpc::ClientContext context;
		ecu_grpc::InterprocMsg request;
		ecu_grpc::InterprocMsg reply;

		grpc::Status status = stub_->sendMsg(&context, request, &reply);
		if (!status.ok()) {
			std::cout << status.error_code() << ": "
				  << status.error_message() << std::endl;
		}
	}
};

int main(int argc, char **argv)
{
	std::this_thread::sleep_for(std::chrono::seconds(1));

	grpc_init();

	HelloClient client;
	client.reset();
	client.offline();
	client.getState();
	client.setState();
	client.ping();
	client.sendMsg();
	std::cout << "Response: "
		  << "\n";

	grpc_shutdown();

	return 0;
}
