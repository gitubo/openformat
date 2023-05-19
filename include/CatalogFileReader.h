#pragma once

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "MessageElement.h"

using json = nlohmann::ordered_json;

/*
enum ValueType {
    UNDEFINED = 0,
    INTEGER = 10,
    UNSIGNED_INTEGER = 11,
    DOUBLE = 20,
    STRING = 30,
    BOOLEAN = 40,
    DICT = 100,
    LIST = 200
};

typedef struct {
  void* ptr;
  ValueType type;
} ValueStruct;
*/

class CatalogFileReader {
private:
    CatalogFileReader(){};
    std::map<std::string, std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>> configMap;
    MessageElementConstraints parseJsonMessageElementConstraints(json);
    std::vector<MessageElementDependency> parseJsonMessageElementDependecies(json);
    MessageElement parseJsonMessageElement(json, const json&);
    std::vector<MessageElement> parseJsonMessageElementStructure(json, const json&);
    std::map<int, MessageElement> parseJsonMessageElementRouting(json, const json&);
    std::map<std::string, std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>> addConfiguration(json, const std::string&);
    void listFilesRecursive(const std::filesystem::path&, std::vector<std::string>&);

public:
    static CatalogFileReader& getInstance() {
        static CatalogFileReader instance;
        return instance;
    }
    CatalogFileReader(CatalogFileReader const&) = delete;
    void operator=(CatalogFileReader const&) = delete;
    void collect(const std::string&);
    std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>* getSchema(const std::string&);
    static std::string printMessageElementList(const std::vector<MessageElement>&);
};

    
