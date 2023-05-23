#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "BitStream.h"
#include "CatalogFileReader.h"
#include "MessageElement.h"
#include "Logger.h"


class Engine {
public:
    Engine(const std::string&, const std::string&, const Schema*);
    const std::string apply();

private:
    void applyPreTransformations();
    void applyPostTransformations();
    void analizeElement(const MessageElement&, const std::string&);
    void analizeStructure(const std::vector<MessageElement>&, const std::string&);
    bool evaluateExistingConditions(const std::vector<MessageElementExistingCondition>&);

    nlohmann::ordered_json jsonFlatten;
    std::unordered_map<std::string, BitStream*> bitStreamMap;
    BitStream* bitStream;
    const Schema* schema;
};