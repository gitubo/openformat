# OpenFormat

The OpenFormat project aims to simplify the analysis of binary messages by eliminating the need to write specific dissectors for each existing binary format. 

Our goal is to accelerate the data analysis process by providing a highly configurable dissector that automatically converts binary messages into a JSON objects, while respecting the original message field structure. That allows users to focus on the data and information contained in the messages, rather than on the technical complexity of their analysis.

With a particular focus on performance and scalability, our dissector is designed to provide low latency and high throughput capacity, making it suitable for the analysis of high-speed data streams, without compromising the quality of the analyzed data. The application provides a gRPC interface.
