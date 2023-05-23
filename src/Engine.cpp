#include "Engine.h"


Engine::Engine(const std::string& base64_str, const std::string& type_, 
//                    const std::pair<std::map<std::string,std::string>,std::vector<MessageElement>>* schema_){
                    const Schema* schema_){
    bitStream = new BitStream(base64_str, type_);
    schema = schema_;
    Logger::getInstance().log("Setting engine input: " + bitStream->toString() + 
                              " (type: <" + bitStream->getType() + 
                              "> of <" + std::to_string(bitStream->getLength()) + "> bits)", Logger::Level::DEBUG);

    Logger::getInstance().log("Setting schema: " + schema->metadata.at("name"), Logger::Level::DEBUG);

}


const std::string Engine::apply(){

    // Analize the bitstream based on <structure> described by the provided schema
    analizeStructure(schema->structure, "");
    if(bitStream->getOffset() < bitStream->getLength()){
        Logger::getInstance().log("Remaining unprocessed bits in the bit stream: "+
                                  std::to_string(bitStream->getLength()-bitStream->getOffset()) + " bit(s) left", Logger::Level::WARNING);
    }

    std::string returnJson = jsonFlatten.unflatten().dump();
    Logger::getInstance().log("UnFlatten JSON: " + returnJson, Logger::Level::DEBUG);
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
            BitStream* bt;
            MessageElement::MessageElementType type_ = element.getType();
            std::string name_ = parentPath;
            if(element.getType() == MessageElement::MessageElementType::MET_EXTENDED){
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
            }
            nlohmann::json jValue;
            switch(type_){
                case MessageElement::MessageElementType::MET_INTEGER:
                    {
                        int value = bt->to_int(element.getBitLength());
                        jValue = value;
                        routingMapKey = value;    
                        break;
                    }
                case MessageElement::MessageElementType::MET_UNSIGNED_INTEGER:
                    {
                        unsigned int value = bt->to_uint(element.getBitLength());
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

            bitStreamMap[name_] = std::move(bt);
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
    int i = 0;
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
