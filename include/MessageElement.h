#pragma once

#include <string>
#include <Logger.h>

class MessageElementExistingCondition {
public:
    enum MessageElementExistingConditionType {
        DCT_UNDEFINED = 0,
        DCT_EQUAL = 1,
        DCT_NOT_EQUAL = 2,
        DCT_GREATER_THAN = 10,
        DCT_GREATER_THAN_OR_EQUAL_TO = 11,
        DCT_LOWER_THAN = 20,
        DCT_LOWER_THAN_OR_EQUAL_TO = 21,
        DCT_BETWEEN = 40,
        DCT_IN = 50,
        DCT_NOT_IN = 51,
        DCT_EXIST = 100
    };
    MessageElementExistingCondition() {}
    MessageElementExistingCondition(const std::string& ref_field_, const std::string& ref_value_,
                                      MessageElementExistingConditionType condition_):
        ref_field(ref_field_), ref_value(ref_value_), condition(condition_), is_set(true) {}
    const bool isSet() const {return is_set;}
    const std::string getRefField() const {return ref_field;};
    const std::string getRefValue() const {return ref_value;};
    const MessageElementExistingConditionType getCondition() const {return condition;};
    friend std::ostream& operator<<(std::ostream& os, const MessageElementExistingCondition& obj) {
        if(obj.is_set){
            os << obj.ref_field << " " << 
                obj.MessageElementExistingConditionTypeToString(obj.condition) << " " << obj.ref_value;
        } else {
            os << "<object not set>";
        }
        return os;
    }
    static MessageElementExistingConditionType stringToMessageElementExistingConditionType(const std::string& str) {
      static const std::map<std::string, MessageElementExistingConditionType> MessageElementExistingConditionTypeMap = {
        {"undefined", MessageElementExistingConditionType::DCT_UNDEFINED}, 
        {"eq", MessageElementExistingConditionType::DCT_EQUAL},
        {"neq", MessageElementExistingConditionType::DCT_NOT_EQUAL},
        {"gt", MessageElementExistingConditionType::DCT_GREATER_THAN},
        {"gte", MessageElementExistingConditionType::DCT_GREATER_THAN_OR_EQUAL_TO},
        {"lt", MessageElementExistingConditionType::DCT_LOWER_THAN},
        {"lte", MessageElementExistingConditionType::DCT_LOWER_THAN_OR_EQUAL_TO},
        {"between", MessageElementExistingConditionType::DCT_BETWEEN},
        {"in", MessageElementExistingConditionType::DCT_IN},
        {"nin", MessageElementExistingConditionType::DCT_NOT_IN},
        {"exist", MessageElementExistingConditionType::DCT_EXIST},
      };

      auto it = MessageElementExistingConditionTypeMap.find(str);
      if (it != MessageElementExistingConditionTypeMap.end()) {
        return it->second;
      } else {
        Logger::getInstance().log("Invalid type provided", Logger::Level::ERROR);
      }
      return MessageElementExistingConditionType::DCT_UNDEFINED;
    }


private:
    bool is_set = false;
    std::string ref_field;
    std::string ref_value;
    MessageElementExistingConditionType condition;
    std::string MessageElementExistingConditionTypeToString(const MessageElementExistingConditionType value) const {
      switch (value) {
        case DCT_UNDEFINED:
          return "undefined";
        case DCT_EQUAL:
          return "equal";
        case DCT_NOT_EQUAL:
          return "not equal";
        case DCT_GREATER_THAN:
          return "greater than";
        case DCT_GREATER_THAN_OR_EQUAL_TO:
          return "greater than or equal to";
        case DCT_LOWER_THAN:
          return "lower than";
        case DCT_LOWER_THAN_OR_EQUAL_TO:
          return "lower than or equal to";
        case DCT_BETWEEN:
          return "between";
        case DCT_IN:
          return "in";
        case DCT_NOT_IN:
          return "not in";
        case DCT_EXIST:
          return "exist";
        default:
          return "UNKNOWN";
      }
    }
};


class MessageElement {
public:
    enum class MessageElementType {
        MET_UNDEFINED = 0,
        MET_EXTENDED = 1,
        MET_INTEGER = 10,
        MET_UNSIGNED_INTEGER = 11,
        MET_DECIMAL = 20,
        MET_STRING = 30,
        MET_BOOLEAN = 40,
        MET_STRUCTURE = 100,
        MET_ROUTING = 200,
        MET_PAYLOAD = 1000
    };
    enum class NumericEncodingType {
        NE_UNDEFINED = 0,
        NE_BINARY = 10,
        NE_BCD = 20,
        NE_ASCII = 30,
        NE_2COMPLEMENT = 40
    };

    MessageElement() {}
    MessageElement(const std::string& name_, size_t bitLength_, int repetitions_, const MessageElementType& type_) : 
      name(name_), bitLength(bitLength_), repetitions(repetitions_), type(type_) {}

    std::string setName(const std::string& name_) {name = name_; return name;}
    size_t setBitLength(size_t bitLength_) {bitLength = bitLength_; return bitLength;}
    int setRepetitions(int repetitions_) {
        repetitions = repetitions_;
        is_array = true;
        return repetitions;
    }
    std::string setExtendElement(const std::string& extend_element_) {extend_element = extend_element_; return extend_element;}
    int setDelimiter(int delimiter_) { 
        delimiter = delimiter_; 
        bitLength = 0;
        return delimiter; 
    }
    bool setVisibility(bool is_visible_) { is_visible = is_visible_; return is_visible_; }
    bool setFlattenStructure(bool is_flatten_structure_) { is_flatten_structure = is_flatten_structure_; return is_flatten_structure; }
    std::string setRepetitionsReference(const std::string repetitionsReference_) {
        repetitionsReference = repetitionsReference_;
        repetitions = 0;
        is_array = true;
        return repetitionsReference;
    }    
    MessageElementType setType(MessageElementType type_) {type = type_; return type;}
    std::vector<MessageElementExistingCondition> setExistingConditions(const std::vector<MessageElementExistingCondition>& existingConditions_) {
        existingConditions = existingConditions_;
        return existingConditions;
    }
    std::vector<MessageElement> setStructure(const std::vector<MessageElement>& structure_) {
        structure = structure_;
        return structure;
    }
    std::map<int, MessageElement> setRouting(const std::map<int, MessageElement>& routing_) {
        routing = routing_;
        return routing;
    }
    NumericEncodingType setNumericEncoding(const NumericEncodingType numeric_encoding_) {numeric_encoding = numeric_encoding_; return numeric_encoding;}
    
    std::string getName() const {return name;}
    size_t getBitLength() const {return bitLength;}
    int getRepetitions() const {return repetitions;}
    std::string getExtendElement() const {return extend_element;}
    int getDelimiter() const {return delimiter;}
    bool isVisible() const {return is_visible;}
    bool isFlattenStructure() const {return is_flatten_structure;}
    std::string getRepetitionsReference() const {return repetitionsReference;}
    MessageElementType getType() const {return type;}
    const std::vector<MessageElementExistingCondition> getExistingConditions() const {return existingConditions;}
    const std::vector<MessageElement> getStructure() const {return structure;}
    const std::map<int, MessageElement> getRouting() const {return routing;}
    NumericEncodingType getNumericEncoding() const {return numeric_encoding;}

    const bool isArray() const {return is_array;} 
    const bool forceIsArray(bool is_array_) const {is_array = is_array_; return is_array;} 

    friend std::ostream& operator<<(std::ostream& os, const MessageElement& obj) {
        os << "{\"name\":\"" << obj.name << "\", \"bit_length\":" << obj.bitLength <<  "\", \"size\":" << obj.repetitions << ", \"type\":\"" << obj.MessageElementTypeToString(obj.type) << "\"";
        if(obj.structure.size()>0){
            os << ", \"structure\":[";
            for(int i = 0; i < obj.structure.size(); i++){
                os << obj.structure[i];
                if(i < obj.structure.size()-1){ os << ", ";}
            }
            os << "]";
        }
        os << "}";
        return os;
    }

    static const MessageElementType stringToMessageElementType(const std::string& str) {
      static const std::map<std::string, MessageElementType> messageElementTypeMap = {
        {"integer", MessageElementType::MET_INTEGER},
        {"unsigned integer", MessageElementType::MET_UNSIGNED_INTEGER},
        {"decimal", MessageElementType::MET_DECIMAL},
        {"string", MessageElementType::MET_STRING},
        {"boolean", MessageElementType::MET_BOOLEAN},
        {"structure", MessageElementType::MET_STRUCTURE},
        {"routing", MessageElementType::MET_ROUTING},
        {"payload", MessageElementType::MET_PAYLOAD},
        {"extended", MessageElementType::MET_EXTENDED},
      };
      auto it = messageElementTypeMap.find(str);
      if (it != messageElementTypeMap.end()) {
        return it->second;
      } 
      return MessageElementType::MET_UNDEFINED;
    }

    static const std::string MessageElementTypeToString(const MessageElementType value) {
      switch (value) {
        case MessageElementType::MET_UNDEFINED:
          return "undefined";
        case MessageElementType::MET_EXTENDED:
          return "extended";
        case MessageElementType::MET_INTEGER:
          return "integer";
        case MessageElementType::MET_UNSIGNED_INTEGER:
          return "unsigned integer";
        case MessageElementType::MET_DECIMAL:
          return "decimal";
        case MessageElementType::MET_STRING:
          return "string";
        case MessageElementType::MET_BOOLEAN:
          return "boolean";
        case MessageElementType::MET_STRUCTURE:
          return "structure";
        case MessageElementType::MET_ROUTING:
          return "routing";
        case MessageElementType::MET_PAYLOAD:
          return "payload";
        default:
          return "UNKNOWN";
      }
    }

    static const NumericEncodingType stringToNumericEncodingType(const std::string& str) {
      static const std::map<std::string, NumericEncodingType> NumericEncodingTypeMap = {
        {"binary", NumericEncodingType::NE_BINARY},
        {"bcd", NumericEncodingType::NE_BCD},
        {"ascii", NumericEncodingType::NE_ASCII},
        {"2complement", NumericEncodingType::NE_2COMPLEMENT},
      };
      auto it = NumericEncodingTypeMap.find(str);
      if (it != NumericEncodingTypeMap.end()) {
        return it->second;
      } 
      return NumericEncodingType::NE_UNDEFINED;
    }

    static const std::string NumericEncodingTypeToString(const NumericEncodingType value) {
      switch (value) {
        case NumericEncodingType::NE_UNDEFINED:
          return "undefined";
        case NumericEncodingType::NE_BINARY:
          return "binary";
        case NumericEncodingType::NE_BCD:
          return "bcd";
        case NumericEncodingType::NE_ASCII:
          return "ascii";
        case NumericEncodingType::NE_2COMPLEMENT:
          return "2complement";
        default:
          return "UNKNOWN";
      }
    }

private:
    std::string name = "<undefined>";
    size_t bitLength = 8;
    int repetitions = 1;
    std::string repetitionsReference = "<undefined>";
    int delimiter = 0;
    NumericEncodingType numeric_encoding = NumericEncodingType::NE_BINARY;

    mutable bool is_array = false;
    mutable bool is_visible = true;
    mutable bool is_flatten_structure = false;
    std::string extend_element = "";
    MessageElementType type;
    std::vector<MessageElementExistingCondition> existingConditions;
    std::vector<MessageElement> structure;
    std::map<int, MessageElement> routing;


};

