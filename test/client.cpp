#include <iostream>
#include <memory>
#include <string>
#include <chrono>


#include <grpcpp/grpcpp.h>
#include "service.grpc.pb.h"
#include "service.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using interface::service;
using interface::toJsonRequest;
using interface::toJsonResponse;

class ServiceClient {
 public:
  ServiceClient(std::shared_ptr<Channel> channel)
      : stub_(service::NewStub(channel)) {}

  std::string toJson(const std::string& message_base64, const std::string& message_type) {
    toJsonRequest request;
    request.set_message_base64(message_base64);
    request.set_message_type(message_type);

    toJsonResponse response;
    ClientContext context;

    auto start_time = std::chrono::high_resolution_clock::now();
    Status status = stub_->toJson(&context, request, &response);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Elaboration time: " << duration.count() << " us" << std::endl;

    if (status.ok()) {
      return response.message_json();
    } else {
      std::cout << "toJson rpc failed: " << status.error_code() << ": " << status.error_message() << std::endl;
      std::cout << "\n\t" << response.response_message() << std::endl;  
      return "";
    }
  }

 private:
  std::unique_ptr<service::Stub> stub_;
};

int main(int argc, char** argv) {

  std::cout << "USAGE: client <host> <port> <message_base64> <message_type>" << std::endl;

    std::string host, port, inputMessageBase64, inputType;
    if (argc > 4) {
        host = argv[1];
        port = argv[2];
        inputMessageBase64 = argv[3];
        inputType = argv[4];
    } else {
        host = "server";
        port = "50051";
        inputMessageBase64 = "BmNpYW8AAAAbQF7dOpKjBVM="; // BmNpYW8AAAAb
        inputType = "test3";
    }

  std::string grpcServer = host + ":" + port;
  ServiceClient client(grpc::CreateChannel(grpcServer, grpc::InsecureChannelCredentials()));

  std::cout << "REQUEST: <" << inputType << "> : " << inputMessageBase64 << std::endl;
  std::string response = client.toJson(inputMessageBase64, inputType);
  std::cout << "RESPONSE: " << response << std::endl;
  return 0;
}