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
    Engine(const std::string&, const std::string&, const std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>*);
    const std::string apply();

private:
    void analizeElement(const MessageElement&, const std::string&);
    void analizeStructure(const std::vector<MessageElement>&, const std::string&);
    template <typename T>
    int validateConstraints(const T&, const MessageElementConstraints&);

    nlohmann::ordered_json jsonFlatten;
    BitStream* bitStream;
    const std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>* schema;
};