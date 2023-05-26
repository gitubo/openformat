#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "BitStream.h"
#include "SchemaCatalog.h"
#include "MessageElement.h"
#include "Logger.h"


class Engine {
public:
    const std::pair<std::string, unsigned int> convertToBinary(const std::string&, const Schema*);
    const std::string convertToJson(const std::string&, const std::string&, const Schema*);

private:
    void analizeElement(const MessageElement&, const std::string&);
    void analizeStructure(const std::vector<MessageElement>&, const std::string&);
    bool evaluateExistingConditions(const std::vector<MessageElementExistingCondition>&);
    void analizeJsonElement(const MessageElement&, const std::string&);
    void analizeJsonStructure(const std::vector<MessageElement>&, const std::string&);
    std::string getTypeString(nlohmann::json::value_t);

    nlohmann::ordered_json jsonFlatten;
    std::unordered_map<std::string, std::unique_ptr<BitStream>> bitStreamMap;
    BitStream* bitStream;
    const Schema* schema;
};