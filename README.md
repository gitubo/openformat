# OpenFormat

The OpenFormat project aims to simplify the analysis of binary messages by eliminating the need to write specific dissectors for each different format. 

Our goal is to accelerate the data analysis process by providing a highly configurable dissector that automatically converts binary messages into JSON objects, while respecting the original message structure. That allows users to focus on the data and information contained in the messages, rather than on the technical complexity of their formats.

With a particular focus on performance and scalability, our dissector is designed to provide low latency and high throughput capacity, making it suitable for the analysis of high-speed data streams, without compromising the quality of the analyzed data. The application provides a gRPC interface.

## Compile

Pre-requirements:
1. gRPC installed
2. nlohmann json library installed

To compile the project:

1. Clone the repository
2. Navigate to the project directory
   $> cd openformat
3. Compile the project using CMake:
   $> mkdir build
   $> cd build
   $> cmake ..
   $> make
4. Run the program:
   $> ./build/openformat
