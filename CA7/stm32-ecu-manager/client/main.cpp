#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

#include <linux/stm32ecu/Interproc_Msg.h>
#include <linux/stm32ecu/stm32ecu.h>

#include "ecu.grpc.pb.h"
#include "ecu.pb.h"
#include "grpcpp/grpcpp.h"

class HelloClient {
private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<helloworld::Greeter::Stub> stub_;

public:
  HelloClient() {
    channel_ = grpc::CreateChannel("192.168.0.4:50051",
                                   grpc::InsecureChannelCredentials());
    stub_ = helloworld::Greeter::NewStub(channel_);
  }

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string &user) {
    helloworld::HelloRequest request;
    request.set_name(user);

    helloworld::HelloReply reply;
    grpc::ClientContext context;
    grpc::Status status = stub_->SayHello(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }

    return reply.message();
  }
};

int main(int argc, char **argv) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  
  grpc_init();
  
  HelloClient client;
  std::cout << "Response: " << client.SayHello("Jordan") << "\n";

  grpc_shutdown();

  return 0;
}
