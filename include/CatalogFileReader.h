#pragma once

#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "MessageElement.h"

using json = nlohmann::ordered_json;


struct Schema{
    std::string version;
    std::map<std::string, std::string> metadata;
    std::vector<MessageElement> structure;
};

class CatalogFileReader {
private:
    CatalogFileReader(){};
    std::map<std::string, std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>> configMap;
    std::map<std::string, Schema> schemaMap;

    std::vector<MessageElementExistingCondition> parseJsonMessageElementExistingConditions(json);
    MessageElement parseJsonMessageElement(json, const json&);
    std::vector<MessageElement> parseJsonMessageElementStructure(json, const json&);
    std::map<int, MessageElement> parseJsonMessageElementRouting(json, const json&);

public:
    static CatalogFileReader& getInstance() {
        static CatalogFileReader instance;
        return instance;
    }
    CatalogFileReader(CatalogFileReader const&) = delete;
    void operator=(CatalogFileReader const&) = delete;
    std::map<std::string, Schema> addConfiguration(const std::string&, const std::string&);
    Schema* getSchema(const std::string&);
    static std::string printMessageElementList(const std::vector<MessageElement>&);
};

    
