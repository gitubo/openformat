#include "Engine.h"
#include "Logger.h"
#include "FileWatcher.h"
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <grpcpp/grpcpp.h>
#include "service.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using interface::toJsonRequest;
using interface::toJsonResponse;
using interface::service;

class ServiceImpl final : public service::Service {
  Status toJson(ServerContext* context, const toJsonRequest* request, toJsonResponse* response) override {
    std::string inputMessageBase64 = request->message_base64();
    std::string inputType = request->message_type();

    Logger::getInstance().log("Base64 input message (type: <" + inputType + ">): "+inputMessageBase64, Logger::Level::INFO);
 
    std::string returnJson;
    try{
        auto start_time = std::chrono::high_resolution_clock::now();
        Engine engine(inputMessageBase64, inputType, CatalogFileReader::getInstance().getSchema(inputType));
        returnJson = engine.apply();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        Logger::getInstance().log("Json: " + returnJson, Logger::Level::DEBUG);
        Logger::getInstance().log("Elaboration time: " + std::to_string(duration.count()) + " us", Logger::Level::INFO);
    } catch (const std::exception& e) {
        Logger::getInstance().log("Engine exception: " + std::string(e.what()), Logger::Level::ERROR);
        response->set_message_json("");
        response->set_response_status(500);
        response->set_response_message(std::string(e.what()));
        grpc::Status(grpc::StatusCode::INTERNAL, std::string(e.what()));
    }

    response->set_message_json(returnJson);
    response->set_response_status(200);
    response->set_response_message("OK");

    return Status::OK;
  }
};

void RunServer(const std::string& service_port) {
  std::string server_address("0.0.0.0:"+service_port);
  ServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  Logger::getInstance().log("Server listening on " + server_address, Logger::Level::INFO);

  server->Wait();
}

int main(int argc, char* argv[]) {

    int opt;
    std::string catalog_path = std::getenv("CATALOG_PATH") ? std::string(std::getenv("CATALOG_PATH")) : "../catalog";
    std::string log_level = std::getenv("LOG_LEVEL") ? std::string(std::getenv("LOG_LEVEL")) : "info";
    std::string service_port = std::getenv("PORT") ? std::string(std::getenv("PORT")) : "50051";

    while ((opt = getopt(argc, argv, "c:l:p:")) != -1) {
        switch (opt) {
            case 'c':
                catalog_path = optarg;
                break;
            case 'p':
                service_port = optarg;
                break;
            case 'l':
                log_level = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -c catalog_path -l log_level -p service_port" << std::endl;
                std::exit(EXIT_FAILURE);
        }
    }
    std::transform(log_level.begin(), log_level.end(), log_level.begin(), ::tolower);

    Logger::Level logger_log_level = Logger::Level::INFO;
    Logger::getInstance().log("Argument catalog_path set to " + catalog_path, Logger::Level::INFO);
    Logger::getInstance().log("Argument log_level set to " + log_level, Logger::Level::INFO);
    Logger::getInstance().log("Argument service_port set to " + service_port, Logger::Level::INFO);

    if(log_level == "debug") logger_log_level = Logger::Level::DEBUG;
    else if(log_level == "warning") logger_log_level = Logger::Level::WARNING;
    else if(log_level == "error") logger_log_level = Logger::Level::ERROR;
    else if(log_level == "critical") logger_log_level = Logger::Level::CRITICAL;
    Logger::getInstance().setLevel(logger_log_level);

    // Start the catalog watcher thread
    FileWatcher watcher(catalog_path);
    watcher.StartWatching();

    RunServer(service_port);
    return 0;

}