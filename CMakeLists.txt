cmake_minimum_required(VERSION 3.0)

project(myproject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 ")

add_library(proto_service STATIC proto/cpp/service.grpc.pb.cc proto/cpp/service.pb.cc)
add_library(nlohmann_json INTERFACE)

add_executable(openformat src/SchemaCatalog.cpp src/Engine.cpp src/main.cpp)

find_package(gRPC CONFIG REQUIRED)

target_include_directories(openformat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/proto/cpp)
target_include_directories(nlohmann_json INTERFACE /app/json-3.11.2/include)

target_link_libraries(openformat PRIVATE proto_service gRPC::grpc++ nlohmann_json)

add_executable(client test/client.cpp)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/proto/cpp)
target_link_libraries(client PRIVATE proto_service gRPC::grpc++)
