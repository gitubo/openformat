#include "CatalogFileReader.h"
#include <fstream>

void CatalogFileReader::listFilesRecursive(const std::filesystem::path& dirPath, std::vector<std::string>& filenames) {
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            filenames.push_back(entry.path().string());
        }
        else if (entry.is_directory()) {
            listFilesRecursive(entry.path(), filenames);
        }
    }
}

void CatalogFileReader::collect(const std::string& catalogPath){
    std::vector<std::string> filenames;
    if (std::filesystem::is_directory(catalogPath)) {
        listFilesRecursive(catalogPath, filenames);
    }
    for (const auto& filename_ : filenames) {
        std::string filename = catalogPath + "/" + filename_;

        std::ifstream json_file(filename);
        if (!json_file.is_open()) {
            Logger::getInstance().log("Impossible to open file <" + filename + ">", Logger::Level::ERROR);
            return;
        }        

        // Parsing del file JSON
        json j;
        try {
            json_file >> j;
        }
        catch (const std::exception& e) {
            Logger::getInstance().log(std::string("Problem during the analisys of json file: ") + e.what(), Logger::Level::ERROR);
            return;
        }

        // Chiusura del file
        json_file.close();

        std::filesystem::path thisPath = filename;
        std::string typeName = thisPath.stem().string();
        addConfiguration(j, typeName);
    }
}

std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>* CatalogFileReader::getSchema(const std::string& name) {
    auto it = configMap.find(name);
    if (it != configMap.end()) {
        return &(it->second);
    } else {
        Logger::getInstance().log("Requested schema <" + name + "> does not exist in the loaded catalog", Logger::Level::WARNING);
    }
    return nullptr;
}

MessageElementConstraints CatalogFileReader::parseJsonMessageElementConstraints(json json_value){
    MessageElementConstraints constraints;
    if(not (json_value.type() == json::value_t::object)){
        Logger::getInstance().log("Provided constraints is not an object: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
        return constraints;
    }
    for (const auto& [key, val] : json_value.items()) {
        if(key=="minimum"){
            double value = 0; 
            if(val.type() == json::value_t::number_integer){
                value = static_cast<double>(val.get<int>());
            } else if(val.type() == json::value_t::number_unsigned){
                value = static_cast<double>(val.get<unsigned int>());
            } else if(val.type() == json::value_t::number_float){
                value = static_cast<double>(val.get<double>());
            } else {
                Logger::getInstance().log("Unsupported key or value, wrong correct type: " + key, Logger::Level::ERROR);
                continue;
            }
            constraints.setMinimum(value);
        } else if(key=="maximum"){
            double value = 0; 
            if(val.type() == json::value_t::number_integer){
                value = static_cast<double>(val.get<int>());
            } else if(val.type() == json::value_t::number_unsigned){
                value = static_cast<double>(val.get<unsigned int>());
            } else if(val.type() == json::value_t::number_float){
                value = static_cast<double>(val.get<double>());
            } else {
                Logger::getInstance().log("Unsupported key or value, wrong correct type: " + key, Logger::Level::ERROR);
                continue;
            }
            constraints.setMaximum(val);
        } else {
            Logger::getInstance().log("Unsupported key or value, wrong correct type: " + key, Logger::Level::ERROR);
            continue;
        }
    }
    return constraints;
}

std::vector<MessageElementDependency> CatalogFileReader::parseJsonMessageElementDependecies(json json_value){
    std::vector<MessageElementDependency> dependencies;
    if(not (json_value.type() == json::value_t::array)){
        Logger::getInstance().log("Provided dependencies is not an array: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
        return dependencies;
    }
    for (const auto &entry : json_value) {
        if(not (entry.type() == json::value_t::object)){
            Logger::getInstance().log("Provided dependency is not an object: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::ERROR);
            continue;
        }
        MessageElementDependency dependency;
        std::vector<MessageElementDependencyCondition> conditions;
        if(entry.contains("conditions") && entry["conditions"].type() == json::value_t::array){
            for (const auto &elem : entry["conditions"]) {
                if(not (elem.type() == json::value_t::object)){
                    std::cerr << "Condition is not an object" << std::endl;
                    continue;
                }
                auto elemObj = elem;
                if(elemObj.contains("ref_field") && elemObj["ref_field"].type() == json::value_t::string &&
                   elemObj.contains("condition") && elemObj["condition"].type() == json::value_t::string && 
                   elemObj.contains("ref_value") && elemObj["ref_value"].type() == json::value_t::string ){
                    const std::string ref_field = elemObj["ref_field"].get<std::string>();
                    const std::string ref_value = elemObj["ref_value"].get<std::string>();
                    const std::string cond = elemObj["condition"].get<std::string>();

                    MessageElementDependencyCondition condition(ref_field,ref_value,
                        MessageElementDependencyCondition::stringToMessageElementDependencyConditionType(cond));
                    conditions.push_back(condition);
                }
            }
        }
        dependency.setConditions(conditions);

        MessageElementDependencyStatement statement;
        if(entry.count("statement") && entry["statement"].type() == json::value_t::object){
            for (const auto& [key, val] : entry["statement"].items()) {
                if(key=="type" && val.type() == json::value_t::string){
                    statement.setType(MessageElementDependencyStatement::stringToMessageElementDependencyStatementType(val.get<std::string>()));
                } else if(key=="message" && val.type() == json::value_t::string){
                    statement.setMessage(val.get<std::string>());
                }
            }
        }
        dependency.setStatement(statement);
        dependencies.push_back(dependency);
    }
    return dependencies;
}

MessageElement CatalogFileReader::parseJsonMessageElement(json json_value, const json& complete_json){
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
        } else if(key=="size"){
            if(val.type() == json::value_t::number_integer || val.type() == json::value_t::number_unsigned) {
                msgElement.setSize(val.get<int>());
            } else if(val.type() == json::value_t::string){
                msgElement.setArraySizeReference(val.get<std::string>());
                msgElement.setSize(0);
            }
        } else if(key=="type" && val.type() == json::value_t::string){
            msgElement.setType(MessageElement::stringToMessageElementType(val.get<std::string>()));
        } else if(key=="constraints" && val.type() == json::value_t::object){
            msgElement.setConstraints(parseJsonMessageElementConstraints(val));
        } else if(key=="structure" && val.type() == json::value_t::array){
            msgElement.setStructure(parseJsonMessageElementStructure(val, complete_json));
        } else if(key=="routing" && val.type() == json::value_t::array){
            msgElement.setRouting(parseJsonMessageElementRouting(val, complete_json));
        } else if(key=="dependencies" && val.type() == json::value_t::array){
            msgElement.setDependencies(parseJsonMessageElementDependecies(val));
        } else if(key=="visible" && val.type() == json::value_t::boolean){
            msgElement.setVisibility(val.get<bool>());
        } else if(key=="flatten_structure" && val.type() == json::value_t::boolean){
            msgElement.setFlattenStructure(val.get<bool>());
        } else {
            Logger::getInstance().log("Unsupported key or value is not of the correct type: " + key, Logger::Level::DEBUG);
            continue;
        }
    }
    return msgElement;
}

std::vector<MessageElement> CatalogFileReader::parseJsonMessageElementStructure(json json_value, const json& complete_json){
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

std::map<int, MessageElement> CatalogFileReader::parseJsonMessageElementRouting(json json_value, const json& complete_json){
    std::map<int, MessageElement> msgRouting;
    if(not (json_value.type() == json::value_t::array)){
        Logger::getInstance().log("Provided routing table is not an array: " + std::to_string(static_cast<int>(json_value.type())), Logger::Level::DEBUG);
        return msgRouting;
    }
    for (const auto &entry : json_value){
        msgRouting[entry[0]] = parseJsonMessageElement(complete_json.at(json::json_pointer(entry[1])), complete_json);
    }
    return msgRouting;
}

std::map<std::string, std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>> CatalogFileReader::addConfiguration(json json_value, const std::string& name){
    std::map<std::string,std::string> metadata;
    std::vector<MessageElement> structure;
    if(json_value.is_object()){
       for (const auto& [key, val] : json_value.items()) {
            if(key=="structure" && val.type() == json::value_t::array){
                structure = parseJsonMessageElementStructure(val, json_value);
            } else if(key=="name" && val.type() == json::value_t::string){
                metadata["name"] = val.get<std::string>();
            } else if(val.type() == json::value_t::null){
                ;
            } else {
                Logger::getInstance().log("Unsupported JSON value type: " + std::to_string(static_cast<int>(json_value.type())) + " for element named <" + key + "> in file <" + name + ">", Logger::Level::DEBUG);
            }
        }
    } else {
        Logger::getInstance().log("Provided JSON is not an object", Logger::Level::ERROR);
    } 
    std::pair<std::map<std::string,std::string>,std::vector<MessageElement>> config;
    config.first = metadata;
    config.second = structure;
    configMap[name] = config;
    return configMap;
}

std::string CatalogFileReader::printMessageElementList(const std::vector<MessageElement>& vec) {
    std::ostringstream oss;
    oss << "{\"structure\" : [";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        oss << *it;
        if((it < vec.end() - 1)){ oss << ", ";}
    }
    oss << "]}" << std::endl;
    return oss.str();
}
