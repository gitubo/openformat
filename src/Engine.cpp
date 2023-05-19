#include "Engine.h"


Engine::Engine(const std::string& base64_str, const std::string& type_, 
                    const std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>* schema_){
    bitStream = new BitStream(base64_str, type_);
    schema = schema_;
    Logger::getInstance().log("Setting engine input: " + bitStream->toString() + 
                              " (type: <" + bitStream->getType() + 
                              "> of <" + std::to_string(bitStream->getLength()) + "> bits)", Logger::Level::DEBUG);

    Logger::getInstance().log("Setting schema: " + schema->first.at("name"), Logger::Level::DEBUG);

}


const std::string Engine::apply(){
    analizeStructure(schema->second, "");

    if(bitStream->getOffset() < bitStream->getLength()){
        Logger::getInstance().log("Remaining unprocessed bits in the bit stream: "+
                                  std::to_string(bitStream->getLength()-bitStream->getOffset()) + " bit(s) left", Logger::Level::WARNING);
    }

    std::string returnJson = jsonFlatten.unflatten().dump();
    Logger::getInstance().log("UnFlatten JSON: " + returnJson, Logger::Level::DEBUG);
    return returnJson;
}

void Engine::analizeElement(const MessageElement& element, const std::string& parentPath) {
    if(element.isArray()){
        size_t repetition = element.getSize(); 
        if(repetition==0) {
            Logger::getInstance().log("Evaluating reference as array size <" + element.getArraySizeReference() + ">", Logger::Level::DEBUG);
            if (jsonFlatten.contains(element.getArraySizeReference())) {
                repetition = jsonFlatten[element.getArraySizeReference()].get<unsigned int>();
            } else {
                Logger::getInstance().log("Array size reference not found or not yet analyzed", Logger::Level::ERROR);
                Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                throw std::invalid_argument("Array size reference <" + element.getArraySizeReference() + "> not found or not yet analyzed");
            }
        }
        Logger::getInstance().log("Array size <" + std::to_string(repetition) + ">", Logger::Level::DEBUG);
        element.forceIsArray(false);  // force to evaluate the element N times
        for(size_t i=0; i < repetition; i++){
            analizeElement(element, parentPath + "/" + std::to_string(i));
        }
    } else {
        int routingMapKey = 0;
        try{
            switch(element.getType()){
                case MessageElement::MessageElementType::MET_INTEGER:
                    {
                        int value = bitStream->consume(element.getBitLength()).to_int(element.getBitLength());
                        if(element.getConstraints().isSet() && validateConstraints(value, element.getConstraints()) != 0){
                            std::string error_message = element.getName() + " does not match constraints";
                            Logger::getInstance().log(error_message, Logger::Level::ERROR);
                            throw std::invalid_argument(error_message);
                        }
                        if(element.isVisible()){ jsonFlatten[parentPath] = value; }
                        routingMapKey = value;    
                        break;
                    }
                case MessageElement::MessageElementType::MET_UNSIGNED_INTEGER:
                    {
                        unsigned int value = bitStream->consume(element.getBitLength()).to_uint(element.getBitLength());
                        if(element.getConstraints().isSet() && validateConstraints(value, element.getConstraints()) != 0){
                            std::string error_message = element.getName() + " does not match constraints";
                            Logger::getInstance().log(error_message, Logger::Level::ERROR);
                            throw std::invalid_argument(error_message);
                        }
                        if(element.isVisible()){ jsonFlatten[parentPath] = value; }
                        routingMapKey = value;    
                        break;
                    }
                case MessageElement::MessageElementType::MET_DECIMAL:
                    {
                        double value = bitStream->consume(element.getBitLength()).to_double(element.getBitLength());
                        if(element.getConstraints().isSet() && validateConstraints(value, element.getConstraints()) != 0){
                            std::string error_message = element.getName() + " does not match constraints";
                            Logger::getInstance().log(error_message, Logger::Level::ERROR);
                            throw std::invalid_argument(error_message);
                        }
                        if(element.isVisible()){ jsonFlatten[parentPath] = value; }
                        break;
                    }
                case MessageElement::MessageElementType::MET_STRING:
                    {
                        std::string value = bitStream->consume(element.getBitLength()).to_string();
                        if(element.getConstraints().isSet() && validateConstraints(value, element.getConstraints()) != 0){
                            std::string error_message = element.getName() + " does not match constraints";
                            Logger::getInstance().log(error_message, Logger::Level::ERROR);
                            throw std::invalid_argument(error_message);
                        }
                        if(element.isVisible()){ jsonFlatten[parentPath] = value; }
                        break;
                    }
                case MessageElement::MessageElementType::MET_BOOLEAN:
                    {
                        bool value = bitStream->consume(element.getBitLength()).to_boolean();
                        if(element.getConstraints().isSet() && validateConstraints(value, element.getConstraints()) != 0){
                            std::string error_message = element.getName() + " does not match constraints";
                            Logger::getInstance().log(error_message, Logger::Level::ERROR);
                            throw std::invalid_argument(error_message);
                        }
                        if(element.isVisible()){ jsonFlatten[parentPath] = value; }
                        break;
                    }
                default:
                    Logger::getInstance().log("Usupported type <" + MessageElement::MessageElementTypeToString(element.getType()) + "> for field with name: " + element.getName(), Logger::Level::ERROR);
                    throw std::invalid_argument("Usupported type <" + MessageElement::MessageElementTypeToString(element.getType()) + "> for field with name: " + element.getName());
                    break; 
            }
        } catch (const std::exception& e) {
            throw;
        }
        if(element.getRouting().size()){
            // There is a routing map that must be analyzed
            if(element.getRouting().count(routingMapKey)){
                const MessageElement elementOfTheMap = element.getRouting().at(routingMapKey);
                //std::string newParentPath = parentPath.substr(0, parentPath.rfind('/')) + "/" + elementOfTheMap.getName();    
                std::string newParentPath = parentPath.substr(0, parentPath.rfind('/'));    
                if(not elementOfTheMap.isFlattenStructure()){
                    newParentPath += "/" + elementOfTheMap.getName();
                } 
                if(elementOfTheMap.getType() == MessageElement::MessageElementType::MET_STRUCTURE){
                    if(elementOfTheMap.isArray()){
                        size_t repetition = elementOfTheMap.getSize(); 
                        if(repetition==0) {
                            Logger::getInstance().log("Evaluating reference as array size <" + elementOfTheMap.getArraySizeReference() + ">", Logger::Level::DEBUG);
                            if (jsonFlatten.contains(elementOfTheMap.getArraySizeReference())) {
                                repetition = jsonFlatten[elementOfTheMap.getArraySizeReference()].get<unsigned int>();
                            } else {
                                Logger::getInstance().log("Array size reference not found or not yet analyzed", Logger::Level::ERROR);
                                Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                                throw std::invalid_argument("Array size reference <" + elementOfTheMap.getArraySizeReference() + "> not found or not yet analyzed");
                            }
                        }
                        Logger::getInstance().log("Array size <" + std::to_string(repetition) + ">", Logger::Level::DEBUG);
                        for(size_t i=0; i < repetition; i++){
                            analizeStructure(elementOfTheMap.getStructure(), newParentPath + "/" + std::to_string(i));
                        }
                    } else {
                        analizeStructure(elementOfTheMap.getStructure(), newParentPath);            
                    }
                } else {
                    analizeElement(elementOfTheMap, newParentPath);            
                }
            } else {
                // Routing key not found in the map
                std::string err_message = "Provided the routing key <" + std::to_string(routingMapKey) + "> that has not been configured for element <" + element.getName() + ">";
                Logger::getInstance().log(err_message, Logger::Level::ERROR);
                throw std::invalid_argument(err_message);            
            }
        }
    }
}

void Engine::analizeStructure(const std::vector<MessageElement>& structure, const std::string& parentPath){
    int i = 0;
    Logger::getInstance().log("Evaluating a structure of "+
                              std::to_string(structure.size()) + " elements", Logger::Level::DEBUG);
    for (auto it = structure.begin(); it != structure.end(); ++it) {
        Logger::getInstance().log("Evaluating element <" + it->getName() +
                                  "> as a <" + MessageElement::MessageElementTypeToString(it->getType()) + ">" +
                                  " of <" + std::to_string(it->getBitLength()) + "> bit(s) from bit <"+ std::to_string(bitStream->getOffset()) + ">", Logger::Level::DEBUG);
        
        if(it->getType() == MessageElement::MessageElementType::MET_STRUCTURE){ 
            if(it->isArray()){
                size_t repetition = it->getSize(); 
                if(repetition==0) {
                    Logger::getInstance().log("Evaluating reference as array size <" + it->getArraySizeReference() + ">", Logger::Level::DEBUG);
                    if (jsonFlatten.contains(it->getArraySizeReference())) {
                        repetition = jsonFlatten[it->getArraySizeReference()].get<unsigned int>();
                    } else {
                        Logger::getInstance().log("Array size reference not found or not yet analyzed", Logger::Level::ERROR);
                        Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                        throw std::invalid_argument("Array size reference <" + it->getArraySizeReference() + "> not found or not yet analyzed");
                    }
                }
                Logger::getInstance().log("Array size <" + std::to_string(repetition) + ">", Logger::Level::DEBUG);
                it->forceIsArray(false);  // force to evaluate the element N times
                for(size_t i=0; i < repetition; i++){
                    analizeStructure(it->getStructure(), parentPath + "/" + std::to_string(i));
                }
            } else {
                std::string newParentPath = parentPath;    
                if(not it->isFlattenStructure()){
                    newParentPath += "/" + it->getName();
                } 
                analizeStructure(it->getStructure(), newParentPath);    
            }        
        } else {
            analizeElement(*it, parentPath + "/" + it->getName());
        }
    }
}

template <typename T>
int Engine::validateConstraints(const T& value, const MessageElementConstraints& constraints){
/*
    Logger::getInstance().log("Validating constrains", logging::trivial::debug);        
    if (std::is_same<T, std::string>::value){
        return 0;
    } else if (std::is_same<T, int>::value || std::is_same<T, unsigned int>::value || std::is_same<T, double>::value){
        if(constraints.isSetMinimum() && constraints.getMinimum() > value){ return 10; }
        if(constraints.isSetMaximum() && constraints.getMaximum() < value){ return 20; }
    } else {
        Logger::getInstance().log("Trying to attend validation on not supported type", logging::trivial::warning);                    
        return 100;
    }
*/
    return 0;
}
