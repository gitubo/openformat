# OpenFormat

The OpenFormat project aims to simplify the analysis of binary messages by eliminating the need to write specific dissectors for each different format. 

Our goal is to accelerate the data analysis process by providing a highly configurable dissector that automatically converts binary messages into JSON objects, while respecting the original message structure. That allows users to focus on the data and information contained in the messages, rather than on the technical complexity of their formats.

With a particular focus on performance and scalability, our dissector is designed to provide low latency and high throughput capacity, making it suitable for the analysis of high-speed data streams, without compromising the quality of the analyzed data.

The service is able to provide a human readable message (json) starting from a stream of bit(provided as a base64-coded string) and viceversa. The mapping between the two types of serialization is provided by a configuration file (json) describing the structure of the message itself. The same configuration file is used to convert a stream of bits into a json and a json into a stream of bits.

## Schema

The mapping between binary and human readable message is provided by a json file named 'schema'. the collection of the needed schemas is named schema catalog and is stored into a directory in the filesystem. The schema catalog can be organized as a set of nested directory.

In order to define the structure of the message, a set of field definitions must be provided. Each field has its own properties, including:
1. the name
2. the length in bit
3. the type
4. additional details specific for each type of field such as the encoding, the structure, any existing condition and so on

### Supported types of fields

Currently the schema supports the following types of fields in a message:
1. integer - a field representing an integer (positive or negative) from 1 to 32 bits
2. unsigned integer - a field representing  a unsigned integer from 1 to 32 bits
3. decimal - a field representing a decimal value with precision of 32 or 64 bits
4. string - a field representing a set of characters (UTF-8)
5. boolean - a field representing a bolean value (0 = FALSE, otherwise TRUE)
6. structure - a field representing a complex object made by multiple subtypes
7. routing - a field mapping set of possile different other objects
8. extended - a field extending the value of another field
9. payload - a filed representing a generic payload (a stream of bits)

### Example of a schema

Here the example of a schema describing the mapping of CAN format (both standard and extended)
```json
{
	"version": "0.1",
	"metadata": {
		"name": "can",
		"description": "CAN standard and extended"
	},
	"structure": [
		{
			"name": "start_of_frame",
			"bit_length": 1,
			"type": "unsigned integer"
		},
		{
			"name": "identifier",
			"bit_length": 11,
			"type": "unsigned integer"
		},
		{
			"name": "RTR",
			"bit_length": 1,
			"type": "unsigned integer"
		},
		{
			"name": "IDE",
			"bit_length": 1,
			"type": "unsigned integer",
			"routing":[
				[0, "/definitions/standard"],
				[1, "/definitions/extended"]
			]				
		}	
	],
	"definitions":{
		"standard":{
			"name": "standard",
			"type": "structure",
			"flatten_structure": true,
			"structure": [
				{
					"name": "Reserved",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "DLC",
					"bit_length": 4,
					"type": "unsigned integer"
				},
				{
					"name": "data",
					"bit_length": 8,
					"type": "unsigned integer",
					"repetitions": "/DLC"
				},
				{
					"name": "CRC",
					"bit_length": 15,
					"type": "unsigned integer"
				},
				{
					"name": "CRC_delimiter",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "ACK",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "ACK_delimiter",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "EOF",
					"bit_length": 7,
					"type": "unsigned integer"
				}
			]
		},
		"extended":{
			"name": "extended",
			"type": "structure",
			"flatten_structure": true,
			"structure": [
				{
					"name": "extended_identifier",
					"bit_length": 18,
					"type": "extended",
					"extend": "/identifier"
				},
				{
					"name": "RTR",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "Reserved0",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "Reserved1",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "DLC",
					"bit_length": 4,
					"type": "unsigned integer"
				},				
				{
					"name": "data",
					"bit_length": 8,
					"type": "unsigned integer",
					"repetitions": "/DLC"
				},
				{
					"name": "CRC",
					"bit_length": 15,
					"type": "unsigned integer"
				},
				{
					"name": "CRC_delimiter",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "ACK",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "ACK_delimiter",
					"bit_length": 1,
					"type": "unsigned integer"
				},
				{
					"name": "EOF",
					"bit_length": 7,
					"type": "unsigned integer"
				}
			]
		}
	}
}
```

## How to compile

### Pre-requirements
1. gRPC installed
2. nlohmann json library installed

### Steps to compile
1. Clone the repository
2. Navigate to the project directory
```sh
cd openformat
```
3. Compile the project using CMake:
```sh
mkdir build
cd build
cmake ..
make
```
4. Run the program:
```sh
./build/openformat
```
