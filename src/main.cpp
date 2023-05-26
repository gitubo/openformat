#include "Engine.h"
#include "Logger.h"
#include "FileWatcher.h"
#include <chrono>
#include <thread>
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
using interface::toBitsRequest;
using interface::toBitsResponse;
using interface::service;

class ServiceImpl final : public service::Service {
  Status toJson(ServerContext* context, const toJsonRequest* request, toJsonResponse* response) override {
    std::string inputMessageBase64 = request->message_base64();
    std::string inputType = request->message_type();

    Logger::getInstance().log("Input message (type: <" + inputType + ">): "+inputMessageBase64, Logger::Level::INFO);
 
    std::string returnJson;
    try{
        Engine engine;
        auto start_time = std::chrono::high_resolution_clock::now();
        returnJson = engine.convertToJson(inputMessageBase64, inputType, SchemaCatalog::getInstance().getSchema(inputType));
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        Logger::getInstance().log("Json: " + returnJson, Logger::Level::DEBUG);
        Logger::getInstance().log("Elaboration time: " + std::to_string(duration.count()) + " us", Logger::Level::INFO);
    } catch (const std::exception& e) {
        Logger::getInstance().log("Engine exception: " + std::string(e.what()), Logger::Level::ERROR);
        response->set_message_json("");
        response->set_message_type("");
        response->set_response_status(500);
        response->set_response_message(std::string(e.what()));
        grpc::Status(grpc::StatusCode::INTERNAL, std::string(e.what()));
    }

    response->set_message_json(returnJson);
    response->set_message_type(inputType);
    response->set_response_status(200);
    response->set_response_message("OK");

    return Status::OK;
  }

  Status toBits(ServerContext* context, const toBitsRequest* request, toBitsResponse* response) override {
    std::string inputMessageJson = request->message_json();
    std::string inputType = request->message_type();

    Logger::getInstance().log("Input message (type: <" + inputType + ">): "+inputMessageJson, Logger::Level::INFO);
 
    std::pair<std::string, unsigned int> returnBase64;
    try{
        Engine engine;
        auto start_time = std::chrono::high_resolution_clock::now();
        returnBase64 = engine.convertToBinary(inputMessageJson, SchemaCatalog::getInstance().getSchema(inputType));
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        Logger::getInstance().log("Bit stream base64: " + returnBase64.first + " (" + std::to_string(returnBase64.second) + " bits)", Logger::Level::DEBUG);
        Logger::getInstance().log("Elaboration time: " + std::to_string(duration.count()) + " us", Logger::Level::INFO);
    } catch (const std::exception& e) {
        Logger::getInstance().log("Engine exception: " + std::string(e.what()), Logger::Level::ERROR);
        response->set_message_base64("");
        response->set_message_length(0);
        response->set_message_type("");
        response->set_response_status(500);
        response->set_response_message(std::string(e.what()));
        grpc::Status(grpc::StatusCode::INTERNAL, std::string(e.what()));
    }

    response->set_message_base64(returnBase64.first);
    response->set_message_length(static_cast<int>(returnBase64.second));
    response->set_message_type(inputType);
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
    std::string input_data = "";

    while ((opt = getopt(argc, argv, "c:l:p:d:")) != -1) {
        switch (opt) {
            case 'c':
                catalog_path = optarg;
                break;
            case 'p':
                service_port = optarg;
                break;
            case 'd':
                input_data = optarg;
                break;
            case 'l':
                log_level = optarg;
                std::transform(log_level.begin(), log_level.end(), log_level.begin(), ::tolower);
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -c catalog_path -l log_level -p service_port -d input_data" << std::endl;
                std::exit(EXIT_FAILURE);
        }
    }

    Logger::Level logger_log_level = Logger::Level::INFO;
    if(log_level == "debug") logger_log_level = Logger::Level::DEBUG;
    else if(log_level == "warning") logger_log_level = Logger::Level::WARNING;
    else if(log_level == "error") logger_log_level = Logger::Level::ERROR;
    else if(log_level == "critical") logger_log_level = Logger::Level::CRITICAL;
    Logger::getInstance().setLevel(logger_log_level);

    // Start the catalog watcher thread
    FileWatcher watcher(catalog_path);

    if(input_data.empty()){
        // Start the application as a server receiving input via gRPC
        Logger::getInstance().log("Application log level: " + log_level, Logger::Level::INFO);
        Logger::getInstance().log("Working catalog path: " + catalog_path, Logger::Level::INFO);
        watcher.StartWatching();
        RunServer(service_port);
        watcher.StopWatching();
    } else {
        // Just analyze the input data string
        Logger::getInstance().setOutput(Logger::Output::FILE);
        Logger::getInstance().log("Application log level: " + log_level, Logger::Level::INFO);
        Logger::getInstance().log("Working catalog path: " + catalog_path, Logger::Level::INFO);
        watcher.loadCatalog();
        Logger::getInstance().log("Analyzing: "+input_data, Logger::Level::INFO);
        std::size_t delimiter_pos = input_data.find(":");
        if (delimiter_pos == std::string::npos) {
            Logger::getInstance().log("Data input without the ':' delimiter. Correct format is <type>:<base64_payload>", Logger::Level::CRITICAL);
            return 1;
        }
        std::string inputType = input_data.substr(0, delimiter_pos);
        std::string inputMessageBase64 = input_data.substr(delimiter_pos + 1);
        if (inputType.empty() || inputMessageBase64.empty()) {
            Logger::getInstance().log("Data input does not contain 'type' or 'base64_payload'. Correct format is <type>:<base64_payload>", Logger::Level::CRITICAL);            
            return 2;
        }
        Engine engine;
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string returnJson = engine.convertToJson(inputMessageBase64, inputType, SchemaCatalog::getInstance().getSchema(inputType));
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        Logger::getInstance().log("Elaboration time: " + std::to_string(duration.count()) + " us", Logger::Level::INFO);
        Logger::getInstance().log("Converted json: "+returnJson, Logger::Level::INFO);
        std::cout << returnJson << std::endl;
    }

    return 0;

}