#include "Engine.h"

const std::pair<std::string, unsigned int> Engine::convertToBinary(const std::string& json_str, const Schema* schema_){
    jsonFlatten = json::parse(json_str).flatten();
    schema = schema_;

    Logger::getInstance().log("Setting engine input: " + jsonFlatten.unflatten().dump(), Logger::Level::DEBUG);
    Logger::getInstance().log("Setting schema: " + schema->catalogName, Logger::Level::DEBUG);

    bitStream = new BitStream();

    analizeJsonStructure(schema->structure, "");
    if(jsonFlatten.size()>0){
        Logger::getInstance().log("Remaining unprocessed keys in the json: "+jsonFlatten.unflatten().dump(), Logger::Level::WARNING);
    }

    return std::make_pair(bitStream->toBase64(),bitStream->getLength());
}

void Engine::analizeJsonElement(const MessageElement& element, const std::string& parentPath) {
    if( !(evaluateExistingConditions(element.getExistingConditions())) ){
        return;
    }

    if(element.isArray()){
        int repetitions = element.getRepetitions(); 
        if(repetitions==0) {
            Logger::getInstance().log("Evaluating reference as array size <" + element.getRepetitionsReference() + ">", Logger::Level::DEBUG);
            if (jsonFlatten.contains(element.getRepetitionsReference())) {
                repetitions = jsonFlatten[element.getRepetitionsReference()].get<unsigned int>();
            } else {
                Logger::getInstance().log("Repetitions reference not found or not yet analyzed", Logger::Level::ERROR);
                Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                throw std::invalid_argument("Repetitions reference <" + element.getRepetitionsReference() + "> not found or not yet analyzed");
            }
        }
        Logger::getInstance().log("Repetitions <" + std::to_string(repetitions) + ">", Logger::Level::DEBUG);
        element.forceIsArray(false);  // force to evaluate the element N times
        for(size_t i=0; i < repetitions || repetitions==-1; i++){
            analizeJsonElement(element, parentPath + "/" + std::to_string(i));
            if(jsonFlatten.size()==0) break;
        }
    } else {
        int routingMapKey = 0;
        try{
            //BitStream* bt;
            std::unique_ptr<BitStream> bt;
            MessageElement::MessageElementType type_ = element.getType();
            std::string name_ = parentPath;
            /*if(element.getType() == MessageElement::MessageElementType::MET_EXTENDED){
                BitStream* origBt = bitStreamMap[element.getExtendElement()];
                BitStream* aux;
                if(element.getBitLength()){
                    aux = new BitStream(bitStream->consume(element.getBitLength()));  
                } else {
                    aux = new BitStream(bitStream->consumeUntill(element.getDelimiter()));
                }
                BitStream combinedBitStream = BitStream::combine(*origBt, *aux);
                bt = new BitStream(combinedBitStream.getData(), combinedBitStream.getLength(), "");
                type_ = MessageElement::MessageElementType::MET_UNSIGNED_INTEGER;
                name_ = element.getExtendElement();
            } else {
                if(element.getBitLength()){
                    bt = new BitStream(bitStream->consume(element.getBitLength()));  
                } else {
                    bt = new BitStream(bitStream->consumeUntill(element.getDelimiter()));
                }
            }*/
            json jValue = jsonFlatten[parentPath];
            switch(type_){
                case MessageElement::MessageElementType::MET_INTEGER:
                    {
                        if(jValue.type() != json::value_t::number_integer &&
                           jValue.type() != json::value_t::number_unsigned){
                            throw std::invalid_argument("Invalid type for element <" + parentPath + ">");
                        }
                        int value = static_cast<int>(jValue.get<int64_t>());
                        unsigned char bytes[sizeof(int)];
                        unsigned int numberOfBytes = element.getBitLength()+7>>3;
                        if(numberOfBytes==2){
                            unsigned char plain[sizeof(int)];
                            std::memcpy(plain, &value, sizeof(int));
                            bytes[0] = plain[1];
                            bytes[1] = plain[0];
                        } else if(numberOfBytes==4) {
                            unsigned char plain[sizeof(int)];
                            std::memcpy(plain, &value, sizeof(int));
                            bytes[0] = plain[3];
                            bytes[1] = plain[2];
                            bytes[2] = plain[1];
                            bytes[3] = plain[0];
                        } else {
                            std::memcpy(bytes, &value, sizeof(int));
                        }
//                        bt = new BitStream(bytes, element.getBitLength(), "");
                        bt = std::make_unique<BitStream>(bytes, element.getBitLength(), "");
                        break;
                    }
                case MessageElement::MessageElementType::MET_UNSIGNED_INTEGER:
                    {
                        if(jValue.type() != json::value_t::number_unsigned){
                            throw std::invalid_argument("Invalid type for element <" + parentPath + ">");
                        }
                        unsigned int value = jValue.get<unsigned int>();
                        unsigned char bytes[sizeof(unsigned int)];
                        unsigned int numberOfBytes = element.getBitLength()+7>>3;
                        if(numberOfBytes==2){
                            unsigned char plain[sizeof(unsigned int)];
                            std::memcpy(plain, &value, sizeof(unsigned int));
                            bytes[0] = plain[1];
                            bytes[1] = plain[0];
                        } else if(numberOfBytes==4) {
                            unsigned char plain[sizeof(unsigned int)];
                            std::memcpy(plain, &value, sizeof(unsigned int));
                            bytes[0] = plain[3];
                            bytes[1] = plain[2];
                            bytes[2] = plain[1];
                            bytes[3] = plain[0];
                        } else {
                            std::memcpy(bytes, &value, sizeof(unsigned int));
                        }
//                        bt = new BitStream(bytes, element.getBitLength(), "");    
                        bt = std::make_unique<BitStream>(bytes, element.getBitLength(), "");
                        break;
                    }
                case MessageElement::MessageElementType::MET_DECIMAL:
                    {
                        if(jValue.type() != json::value_t::number_float){
                            throw std::invalid_argument("Invalid type for element <" + parentPath + ">");
                        }
                        if(element.getBitLength()==32){
                            float value = static_cast<float>(jValue.get<double>());
                            unsigned char bytes1[4];
                            unsigned char bytes[4];
                            std::memcpy(bytes1, &value, 4);
                            std::reverse_copy(bytes1, bytes1 + 4, bytes);
//                            bt = new BitStream(bytes, element.getBitLength(), "");
                            bt = std::make_unique<BitStream>(bytes, element.getBitLength(), "");
                        } else if(element.getBitLength()==64){
                            double value = static_cast<double>(jValue.get<double>());
                            unsigned char bytes1[8];
                            unsigned char bytes[8];
                            std::memcpy(bytes1, &value, 8);
                            std::reverse_copy(bytes1, bytes1 + 8, bytes);
//                            bt = new BitStream(bytes, element.getBitLength(), "");
                            bt = std::make_unique<BitStream>(bytes, element.getBitLength(), "");
                        } 
                        break;
                    }
                case MessageElement::MessageElementType::MET_STRING:
                    {
                        if(jValue.type() != json::value_t::string){
                            throw std::invalid_argument("Invalid type for element <" + parentPath + ">");
                        }
                        std::string value = jValue.get<std::string>();
                        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(value.data());
//                        bt = new BitStream(bytes, element.getBitLength(), "");
                        bt = std::make_unique<BitStream>(bytes, element.getBitLength(), "");
                        break;
                    }
                case MessageElement::MessageElementType::MET_BOOLEAN:
                    {
                        if(jValue.type() != json::value_t::boolean){
                            throw std::invalid_argument("Invalid type for element <" + parentPath + ">");
                        }
                        unsigned char bytes = 0b00000000;
                        if(jValue.get<bool>()){
                            bytes = 0b00000001;
                        }
                        bt = std::make_unique<BitStream>(&bytes, element.getBitLength(), "");
                        break;
                    }
                default:
                    Logger::getInstance().log("Usupported type <" + MessageElement::MessageElementTypeToString(type_) + "> for field with name: " + element.getName(), Logger::Level::ERROR);
                    throw std::invalid_argument("Usupported type <" + MessageElement::MessageElementTypeToString(type_) + "> for field with name: " + element.getName());
                    break; 
            }
            if(element.isDelimited()){
                //Append the delimiter (only 1 char is supported)
                unsigned char delimiter_char = static_cast<unsigned char>(element.getDelimiter());
                BitStream* delimiter = new BitStream(&delimiter_char,8,"");
                bt->append(delimiter);
            }
            bitStream->append(bt.get());
            bitStreamMap.emplace(name_,std::move(bt));
            
        } catch (const std::exception& e) {
            throw;
        }
        /*if(element.getRouting().size()){
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
                        int repetitions = elementOfTheMap.getRepetitions(); 
                        if(repetitions==0) {
                            Logger::getInstance().log("Evaluating reference as array size <" + elementOfTheMap.getRepetitionsReference() + ">", Logger::Level::DEBUG);
                            if (jsonFlatten.contains(elementOfTheMap.getRepetitionsReference())) {
                                repetitions = jsonFlatten[elementOfTheMap.getRepetitionsReference()].get<unsigned int>();
                            } else {
                                Logger::getInstance().log("Repetitions reference not found or not yet analyzed", Logger::Level::ERROR);
                                Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                                throw std::invalid_argument("Repetitions reference <" + elementOfTheMap.getRepetitionsReference() + "> not found or not yet analyzed");
                            }
                        }
                        Logger::getInstance().log("Repetitions <" + std::to_string(repetitions) + ">", Logger::Level::DEBUG);
                        for(size_t i=0; i < repetitions || repetitions==-1; i++){
                            analizeStructure(elementOfTheMap.getStructure(), newParentPath + "/" + std::to_string(i));
                            if(not bitStream->remainingBits()) break;
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
        }*/
    }
}

void Engine::analizeJsonStructure(const std::vector<MessageElement>& structure, const std::string& parentPath){
    Logger::getInstance().log("Evaluating a structure of "+
                              std::to_string(structure.size()) + " elements", Logger::Level::DEBUG);
    for (auto it = structure.begin(); it != structure.end(); ++it) {
        Logger::getInstance().log("Evaluating element <" + it->getName() +
                                  "> as a <" + MessageElement::MessageElementTypeToString(it->getType()) + ">" +
                                  " of <" + std::to_string(it->getBitLength()) + "> bit(s) from bit <"+ std::to_string(bitStream->getOffset()) + ">", Logger::Level::DEBUG);
        if(it->getType() == MessageElement::MessageElementType::MET_STRUCTURE){ 
            if(it->isArray()){
                int repetitions = it->getRepetitions(); 
                if(repetitions==0) {
                    Logger::getInstance().log("Evaluating reference as array repetitions <" + it->getRepetitionsReference() + ">", Logger::Level::DEBUG);
                    if (jsonFlatten.contains(it->getRepetitionsReference())) {
                        repetitions = jsonFlatten[it->getRepetitionsReference()].get<unsigned int>();
                    } else {
                        Logger::getInstance().log("Repetitions reference not found or not yet analyzed", Logger::Level::ERROR);
                        Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                        throw std::invalid_argument("Repetitions reference <" + it->getRepetitionsReference() + "> not found or not yet analyzed");
                    }
                }
                Logger::getInstance().log("Repetitions <" + std::to_string(repetitions) + ">", Logger::Level::DEBUG);
                it->forceIsArray(false);  // force to evaluate the element N times
                for(int i=0; i < repetitions || repetitions==-1; i++){
                    analizeJsonStructure(it->getStructure(), parentPath + "/" + std::to_string(i));
                    if(jsonFlatten.size()==0) break;
                }
            } else {
                std::string newParentPath = parentPath;    
                if(not it->isFlattenStructure()){
                    newParentPath += "/" + it->getName();
                } 
                analizeJsonStructure(it->getStructure(), newParentPath);    
            }        
        } else {
            analizeJsonElement(*it, parentPath + "/" + it->getName());
        }
    }
}

const std::string Engine::convertToJson(const std::string& base64_str, const std::string& type_,  const Schema* schema_){

    // Analize the bitstream based on <structure> described by the provided schema
    bitStream = new BitStream(base64_str, type_);
    schema = schema_;
    analizeStructure(schema->structure, "");

    if(bitStream->getOffset() < bitStream->getLength()){
        Logger::getInstance().log("Remaining unprocessed bits in the bit stream: "+
                                  std::to_string(bitStream->getLength()-bitStream->getOffset()) + " bit(s) left", Logger::Level::WARNING);
    }

    std::string returnJson = jsonFlatten.unflatten().dump();
    return returnJson;
}

void Engine::analizeElement(const MessageElement& element, const std::string& parentPath) {
    if( !(evaluateExistingConditions(element.getExistingConditions())) ){
        return;
    }

    if(element.isArray()){
        int repetitions = element.getRepetitions(); 
        if(repetitions==0) {
            Logger::getInstance().log("Evaluating reference as array size <" + element.getRepetitionsReference() + ">", Logger::Level::DEBUG);
            if (jsonFlatten.contains(element.getRepetitionsReference())) {
                repetitions = jsonFlatten[element.getRepetitionsReference()].get<unsigned int>();
            } else {
                Logger::getInstance().log("Repetitions reference not found or not yet analyzed", Logger::Level::ERROR);
                Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                throw std::invalid_argument("Repetitions reference <" + element.getRepetitionsReference() + "> not found or not yet analyzed");
            }
        }
        Logger::getInstance().log("Repetitions <" + std::to_string(repetitions) + ">", Logger::Level::DEBUG);
        element.forceIsArray(false);  // force to evaluate the element N times
        for(size_t i=0; i < repetitions || repetitions==-1; i++){
            analizeElement(element, parentPath + "/" + std::to_string(i));
            if(not bitStream->remainingBits()) break;
        }
    } else {
        int routingMapKey = 0;
        try{
//            BitStream* bt;
            std::unique_ptr<BitStream> bt;
            MessageElement::MessageElementType type_ = element.getType();
            std::string name_ = parentPath;
            if(element.getType() == MessageElement::MessageElementType::MET_EXTENDED){
                BitStream* origBt = bitStreamMap[element.getExtendElement()].get();
                BitStream* aux;
                if(element.getBitLength()){
                    aux = new BitStream(bitStream->consume(element.getBitLength()));  
                } else {
                    aux = new BitStream(bitStream->consumeUntill(element.getDelimiter()));
                }
                BitStream combinedBitStream = BitStream::combine(*origBt, *aux);
//                bt = new BitStream(combinedBitStream.getData(), combinedBitStream.getLength(), "");
                bt = std::make_unique<BitStream>(combinedBitStream.getData(), combinedBitStream.getLength(), "");
                type_ = MessageElement::MessageElementType::MET_UNSIGNED_INTEGER;
                name_ = element.getExtendElement();
            } else {
                if(element.getBitLength()){
//                    bt = new BitStream(bitStream->consume(element.getBitLength()));  
                    bt = std::make_unique<BitStream>(bitStream->consume(element.getBitLength()));
                } else {
//                    bt = new BitStream(bitStream->consumeUntill(element.getDelimiter()));
                    bt = std::make_unique<BitStream>(bitStream->consumeUntill(element.getBitLength()));
                }
            }
            nlohmann::json jValue;
            switch(type_){
                case MessageElement::MessageElementType::MET_INTEGER:
                    {
                        int value;
                        if(element.getNumericEncoding() == MessageElement::NumericEncodingType::NE_BCD){
                            value = bt->to_int_bcd(element.getBitLength());
                        } else {
                            value = bt->to_int(element.getBitLength());
                        }
                        jValue = value;
                        routingMapKey = value;    
                        break;
                    }
                case MessageElement::MessageElementType::MET_UNSIGNED_INTEGER:
                    {
                        int value;
                        if(element.getNumericEncoding() == MessageElement::NumericEncodingType::NE_BCD){
                            value = bt->to_int_bcd(element.getBitLength());
                        } else {
                            value = bt->to_uint(element.getBitLength());
                        }
                        jValue = value;
                        routingMapKey = value;    
                        break;
                    }
                case MessageElement::MessageElementType::MET_DECIMAL:
                    {
                        double value = bt->to_double(element.getBitLength());
                        jValue = value;
                        break;
                    }
                case MessageElement::MessageElementType::MET_STRING:
                    {
                        std::string value = bt->to_string();
                        jValue = value;
                        break;
                    }
                case MessageElement::MessageElementType::MET_BOOLEAN:
                    {
                        bool value = bt->to_boolean();
                        jValue = value;
                        break;
                    }
                default:
                    Logger::getInstance().log("Usupported type <" + MessageElement::MessageElementTypeToString(type_) + "> for field with name: " + element.getName(), Logger::Level::ERROR);
                    throw std::invalid_argument("Usupported type <" + MessageElement::MessageElementTypeToString(type_) + "> for field with name: " + element.getName());
                    break; 
            }
            bitStreamMap.emplace(name_,std::move(bt));
//            bitStreamMap[name_] = std::move(bt);
            if(element.isVisible()){ jsonFlatten[name_] = jValue; }

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
                        int repetitions = elementOfTheMap.getRepetitions(); 
                        if(repetitions==0) {
                            Logger::getInstance().log("Evaluating reference as array size <" + elementOfTheMap.getRepetitionsReference() + ">", Logger::Level::DEBUG);
                            if (jsonFlatten.contains(elementOfTheMap.getRepetitionsReference())) {
                                repetitions = jsonFlatten[elementOfTheMap.getRepetitionsReference()].get<unsigned int>();
                            } else {
                                Logger::getInstance().log("Repetitions reference not found or not yet analyzed", Logger::Level::ERROR);
                                Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                                throw std::invalid_argument("Repetitions reference <" + elementOfTheMap.getRepetitionsReference() + "> not found or not yet analyzed");
                            }
                        }
                        Logger::getInstance().log("Repetitions <" + std::to_string(repetitions) + ">", Logger::Level::DEBUG);
                        for(size_t i=0; i < repetitions || repetitions==-1; i++){
                            analizeStructure(elementOfTheMap.getStructure(), newParentPath + "/" + std::to_string(i));
                            if(not bitStream->remainingBits()) break;
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

bool Engine::evaluateExistingConditions(const std::vector<MessageElementExistingCondition>& conditions){
    if(conditions.size()==0){
        return true;
    }
    for(auto& condition : conditions){
        if(jsonFlatten.count(condition.getRefField())!=0){
            nlohmann::json field = jsonFlatten[condition.getRefField()];
            nlohmann::json value = condition.getRefValue();
            if(condition.getCondition() == MessageElementExistingCondition::MessageElementExistingConditionType::DCT_EQUAL &&
                jsonFlatten[condition.getRefField()].get<bool>()==false ){
                Logger::getInstance().log("Condition not met on field: " + condition.getRefField(), Logger::Level::DEBUG);
                return false;
            }
        } else if(condition.getCondition() == MessageElementExistingCondition::MessageElementExistingConditionType::DCT_EXIST){
            return false;
        }
    }
    return true;
}

void Engine::analizeStructure(const std::vector<MessageElement>& structure, const std::string& parentPath){
    Logger::getInstance().log("Evaluating a structure of "+
                              std::to_string(structure.size()) + " elements", Logger::Level::DEBUG);
    for (auto it = structure.begin(); it != structure.end(); ++it) {
        Logger::getInstance().log("Evaluating element <" + it->getName() +
                                  "> as a <" + MessageElement::MessageElementTypeToString(it->getType()) + ">" +
                                  " of <" + std::to_string(it->getBitLength()) + "> bit(s) from bit <"+ std::to_string(bitStream->getOffset()) + ">", Logger::Level::DEBUG);
        
        if(it->getType() == MessageElement::MessageElementType::MET_STRUCTURE){ 
            if(it->isArray()){
                int repetitions = it->getRepetitions(); 
                if(repetitions==0) {
                    Logger::getInstance().log("Evaluating reference as array repetitions <" + it->getRepetitionsReference() + ">", Logger::Level::DEBUG);
                    if (jsonFlatten.contains(it->getRepetitionsReference())) {
                        repetitions = jsonFlatten[it->getRepetitionsReference()].get<unsigned int>();
                    } else {
                        Logger::getInstance().log("Repetitions reference not found or not yet analyzed", Logger::Level::ERROR);
                        Logger::getInstance().log(jsonFlatten.dump(), Logger::Level::DEBUG);
                        throw std::invalid_argument("Repetitions reference <" + it->getRepetitionsReference() + "> not found or not yet analyzed");
                    }
                }
                Logger::getInstance().log("Repetitions <" + std::to_string(repetitions) + ">", Logger::Level::DEBUG);
                it->forceIsArray(false);  // force to evaluate the element N times
                for(int i=0; i < repetitions || repetitions==-1; i++){
                    analizeStructure(it->getStructure(), parentPath + "/" + std::to_string(i));
                    if(not bitStream->remainingBits()) break;
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

std::string Engine::getTypeString(nlohmann::json::value_t type) {
    switch (type) {
        case nlohmann::json::value_t::null:
            return "null";
        case nlohmann::json::value_t::object:
            return "object";
        case nlohmann::json::value_t::array:
            return "array";
        case nlohmann::json::value_t::string:
            return "string";
        case nlohmann::json::value_t::boolean:
            return "boolean";
        case nlohmann::json::value_t::number_integer:
            return "integer";
        case nlohmann::json::value_t::number_unsigned:
            return "unsigned integer";
        case nlohmann::json::value_t::number_float:
            return "floating point number";
        default:
            return "unknown";
    }
}