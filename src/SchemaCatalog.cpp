#include "SchemaCatalog.h"

Schema* SchemaCatalog::getSchema(const std::string& name) {
    auto it = schemaMap.find(name);
    if (it != schemaMap.end()) {
        return &(it->second);
    } else {
        Logger::getInstance().log("Requested schema <" + name + "> does not exist in the loaded catalog", Logger::Level::WARNING);
    }
    return nullptr;
}


std::vector<MessageElementExistingCondition> SchemaCatalog::parseJsonMessageElementExistingConditions(json json_value){
    std::vector<MessageElementExistingCondition> conditions;
    if(not (json_value.type() == json::value_t::array)){
        Logger::getInstance().log("Provided existing conditions are not in an array: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
        return conditions;
    }
    for (const auto &elem : json_value) {
        if(not (elem.type() == json::value_t::object)){
            Logger::getInstance().log("Provided condition is not an object: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
            continue;
        }
        std::string ref_field;
        std::string ref_value;
        std::string cond;
        if(elem.contains("ref_field") && elem["ref_field"].type() == json::value_t::string){
            ref_field = elem["ref_field"].get<std::string>();
        }
        if(elem.contains("condition") && elem["condition"].type() == json::value_t::string){
            cond = elem["condition"].get<std::string>();
        }
        if(elem.contains("ref_value") && elem["ref_value"].type() == json::value_t::string){
            ref_value = elem["ref_value"].get<std::string>();
        }
        MessageElementExistingCondition condition(ref_field, ref_value,
            MessageElementExistingCondition::stringToMessageElementExistingConditionType(cond));
        conditions.push_back(condition);
    }
    return conditions;
}


MessageElement SchemaCatalog::parseJsonMessageElement(json json_value, const json& complete_json){
    MessageElement msgElement;
    if(not (json_value.type() == json::value_t::object)){
        Logger::getInstance().log("Provided message element is not an object: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
        return msgElement;
    }
    for (const auto& [key, val] : json_value.items()) {
        
        if(key=="name" && val.type() == json::value_t::string){  
            msgElement.setName(val.get<std::string>());
        } else if(key=="bit_length" && (val.type() == json::value_t::number_integer || val.type() == json::value_t::number_unsigned)){
            msgElement.setBitLength(val.get<int>());
        } else if(key=="repetitions"){
            if(val.type() == json::value_t::number_integer || val.type() == json::value_t::number_unsigned) {
                msgElement.setRepetitions(val.get<int>());
            } else if(val.type() == json::value_t::string){
                msgElement.setRepetitionsReference(val.get<std::string>());
                msgElement.setRepetitions(0);
            }
        } else if(key=="type" && val.type() == json::value_t::string){
            msgElement.setType(MessageElement::stringToMessageElementType(val.get<std::string>()));
        } else if(key=="delimiter" && (val.type() == json::value_t::number_integer || val.type() == json::value_t::number_unsigned)){
            msgElement.setDelimiter(val.get<int>());
        } else if(key=="extend" && val.type() == json::value_t::string){
            msgElement.setExtendElement(val.get<std::string>());
        } else if(key=="structure" && val.type() == json::value_t::array){
            msgElement.setStructure(parseJsonMessageElementStructure(val, complete_json));
        } else if(key=="routing" && val.type() == json::value_t::array){
            msgElement.setRouting(parseJsonMessageElementRouting(val, complete_json));
        } else if(key=="existing_conditions" && val.type() == json::value_t::array){
            msgElement.setExistingConditions(parseJsonMessageElementExistingConditions(val));
        } else if(key=="visible" && val.type() == json::value_t::boolean){
            msgElement.setVisibility(val.get<bool>());
        } else if(key=="numeric_encoding" && val.type() == json::value_t::string){
            msgElement.setNumericEncoding(MessageElement::stringToNumericEncodingType(val.get<std::string>()));
        } else if(key=="flatten_structure" && val.type() == json::value_t::boolean){
            msgElement.setFlattenStructure(val.get<bool>());
        } else {
            Logger::getInstance().log("Unsupported key or value is not of the correct type: " + key, Logger::Level::DEBUG);
            continue;
        }
    }
    return msgElement;
}

std::vector<MessageElement> SchemaCatalog::parseJsonMessageElementStructure(json json_value, const json& complete_json){
    std::vector<MessageElement> msgStructure;
    if(not (json_value.type() == json::value_t::array)){
        Logger::getInstance().log("Provided structure is not an array: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::DEBUG);
        return msgStructure;
    }
    for (const auto &entry : json_value) {
        msgStructure.push_back(parseJsonMessageElement(entry, complete_json));
    }
    return msgStructure;
}

std::map<int, MessageElement> SchemaCatalog::parseJsonMessageElementRouting(json json_value, const json& complete_json){
    std::map<int, MessageElement> msgRouting;
    if(not (json_value.type() == json::value_t::array)){
        Logger::getInstance().log("Provided routing table is not an array: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
        return msgRouting;
    }
    for (const auto &entry : json_value){
        msgRouting[entry[0]] = parseJsonMessageElement(complete_json.at(json::json_pointer(entry[1])), complete_json);
    }
    return msgRouting;
}

std::map<std::string, Schema> SchemaCatalog::addConfiguration(const std::string& file_str, const std::string& name){
    json json_value = json::parse(file_str);
    Schema schema;
    schema.catalogName = name;
    if(json_value.is_object()){
       for (const auto& [key, val] : json_value.items()) {
            if(key=="structure" && val.type() == json::value_t::array){
                schema.structure = parseJsonMessageElementStructure(val, json_value);
            } else if(key=="metadata" && val.type() == json::value_t::object){
                std::map<std::string, std::string> metadata;
                for(auto& [key, val] : val.items()){
                    if(val.type() == json::value_t::string){
                        metadata[key] = val.get<std::string>();
                    } else {
                        Logger::getInstance().log("Unsupported type for element <" + key + ">: elements in <metadata> section can be only string", Logger::Level::WARNING);
                    }
                }
                schema.metadata = metadata;
            } else if(key=="version" && val.type() == json::value_t::string){
                schema.version = val.get<std::string>();
            } else {
                Logger::getInstance().log("Unsupported type: " + std::to_string(static_cast<int>(json_value.type())) + " for element named <" + key + "> in file <" + name + ">", Logger::Level::WARNING);
            }
        }
    } else {
        Logger::getInstance().log("Provided JSON is not an object", Logger::Level::ERROR);
    } 
    schemaMap[name] = schema;
    return schemaMap;
}

std::string SchemaCatalog::printMessageElementList(const std::vector<MessageElement>& vec) {
    std::ostringstream oss;
    oss << "{\"structure\" : [";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        oss << *it;
        if((it < vec.end() - 1)){ oss << ", ";}
    }
    oss << "]}" << std::endl;
    return oss.str();
}
