#pragma once

#include <string>
#include <Logger.h>

class MessageElementDependencyCondition {
public:
    enum MessageElementDependencyConditionType {
        DCT_UNDEFINED = 0,
        DCT_EQUAL = 1,
        DCT_NOT_EQUAL = 2,
        DCT_GREATER_THAN = 10,
        DCT_GREATER_THAN_OR_EQUAL_TO = 11,
        DCT_LOWER_THAN = 20,
        DCT_LOWER_THAN_OR_EQUAL_TO = 21,
        DCT_BETWEEN = 40,
        DCT_IN = 50,
        DCT_NOT_IN = 51
    };
    MessageElementDependencyCondition() {}
    MessageElementDependencyCondition(const std::string& ref_field_, const std::string& ref_value_,
                                      MessageElementDependencyConditionType condition_):
        ref_field(ref_field_), ref_value(ref_value_), condition(condition_), is_set(true) {}
    const bool isSet() const {return is_set;}
    friend std::ostream& operator<<(std::ostream& os, const MessageElementDependencyCondition& obj) {
        if(obj.is_set){
            os << obj.ref_field << " " << 
                obj.MessageElementDependencyConditionTypeToString(obj.condition) << " " << obj.ref_value;
        } else {
            os << "<object not set>";
        }
        return os;
    }
    static MessageElementDependencyConditionType stringToMessageElementDependencyConditionType(const std::string& str) {
      static const std::map<std::string, MessageElementDependencyConditionType> MessageElementDependencyConditionTypeMap = {
        {"undefined", MessageElementDependencyConditionType::DCT_UNDEFINED}, 
        {"eq", MessageElementDependencyConditionType::DCT_EQUAL},
        {"neq", MessageElementDependencyConditionType::DCT_NOT_EQUAL},
        {"gt", MessageElementDependencyConditionType::DCT_GREATER_THAN},
        {"gte", MessageElementDependencyConditionType::DCT_GREATER_THAN_OR_EQUAL_TO},
        {"lt", MessageElementDependencyConditionType::DCT_LOWER_THAN},
        {"lte", MessageElementDependencyConditionType::DCT_LOWER_THAN_OR_EQUAL_TO},
        {"between", MessageElementDependencyConditionType::DCT_BETWEEN},
        {"in", MessageElementDependencyConditionType::DCT_IN},
        {"nin", MessageElementDependencyConditionType::DCT_NOT_IN},
      };

      auto it = MessageElementDependencyConditionTypeMap.find(str);
      if (it != MessageElementDependencyConditionTypeMap.end()) {
        return it->second;
      } else {
        Logger::getInstance().log("Invalid type provided", Logger::Level::ERROR);
      }
      return MessageElementDependencyConditionType::DCT_UNDEFINED;
    }


private:
    bool is_set = false;
    std::string ref_field;
    std::string ref_value;
    MessageElementDependencyConditionType condition;
    std::string MessageElementDependencyConditionTypeToString(const MessageElementDependencyConditionType value) const {
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
        default:
          return "UNKNOWN";
      }
    }
};

class MessageElementDependencyStatement {
public:
    enum MessageElementDependencyStatementType {
        DST_UNDEFINED = 0,
        DST_EXIST = 10,
        DST_NOT_EXIST = 11,
        DST_ERROR = 100
    };
    MessageElementDependencyStatement() {}
    MessageElementDependencyStatement(MessageElementDependencyStatementType type_, const std::string& message_) :
        type(type_), message(message_), is_set(true) {}
    MessageElementDependencyStatementType setType(MessageElementDependencyStatementType type_){
        type = type_;
        is_set = true; 
        return type;
    }
    const bool isSet() const {return is_set;}
    std::string setMessage(std::string message_){message = message_; return message;}
    MessageElementDependencyStatementType getType(){return type;}
    std::string getMessage(){return message;}
    friend std::ostream& operator<<(std::ostream& os, const MessageElementDependencyStatement& obj) {
        if(obj.is_set){
            os << "\"statement\": {\"type\":\"" << 
                  obj.MessageElementDependencyStatementTypeToString(obj.type) << 
                  "\", \"message\":\"" << obj.message << "\"}";
        } else {
            os << "<object not set>";
        }
        return os;
    }
    static MessageElementDependencyStatementType stringToMessageElementDependencyStatementType(const std::string& str) {
      static const std::map<std::string, MessageElementDependencyStatementType> messageElementDependencyStatementTypeMap = {
        {"undefined", MessageElementDependencyStatementType::DST_UNDEFINED}, 
        {"exist", MessageElementDependencyStatementType::DST_EXIST},
        {"not_exist", MessageElementDependencyStatementType::DST_NOT_EXIST},
        {"error", MessageElementDependencyStatementType::DST_ERROR},
      };
      auto it = messageElementDependencyStatementTypeMap.find(str);
      if (it != messageElementDependencyStatementTypeMap.end()) {
        return it->second;
      } else {
        Logger::getInstance().log("Invalid type provided " + str, Logger::Level::ERROR);
      }
      return MessageElementDependencyStatementType::DST_UNDEFINED;
    }

private:
    bool is_set = false;
    MessageElementDependencyStatementType type;
    std::string message;
    std::string MessageElementDependencyStatementTypeToString(const MessageElementDependencyStatementType value) const {
      switch (value) {
        case DST_UNDEFINED:
          return "undefined";
        case DST_EXIST:
          return "exist";
        case DST_NOT_EXIST:
          return "not exist";
        case DST_ERROR:
          return "error";
        default:
          return "UNKNOWN";
      }
    }
};

class MessageElementDependency {
public:
    MessageElementDependency() {}
    MessageElementDependency(const std::vector<MessageElementDependencyCondition>& conditions_,
                             const MessageElementDependencyStatement& statement_) :
        conditions(conditions_), statement(statement_), is_set(true){}
    std::vector<MessageElementDependencyCondition> setConditions(const std::vector<MessageElementDependencyCondition>& conditions_) {
        conditions = conditions_; 
        is_set = true;
        return conditions;
    }
    const bool isSet() const {return is_set;}
    MessageElementDependencyStatement setStatement(const MessageElementDependencyStatement& statement_) {statement = statement_; return statement;}
    const std::vector<MessageElementDependencyCondition> getConditions() const {return conditions;}
    const MessageElementDependencyStatement getStatement() const {return statement;}
    friend std::ostream& operator<<(std::ostream& os, const MessageElementDependency& obj) {
        if(obj.is_set){
            os << "{ \"conditions\": [";
            for(int i = 0; i < obj.conditions.size(); i++){
                os << obj.conditions[i];
                if(i < obj.conditions.size()-1){ os << ", ";}
            }
            os << "], " << obj.statement << "}";
        } else {
            os << "<object not set>";
        }
        return os;
    }
private:
    bool is_set = false;
    std::vector<MessageElementDependencyCondition> conditions;
    MessageElementDependencyStatement statement;
};

class MessageElementConstraints {
public:
    MessageElementConstraints() {}
    MessageElementConstraints(double minimum_, double maximum_) : minimum(minimum_), maximum(maximum_), is_set(true) {}
    const bool isSet() const { return is_set; }
    const bool isSetMinimum() const {return is_set_minimum;}
    const bool isSetMaximum() const {return is_set_maximum;}
    int setMinimum(int value) {minimum = value; is_set = true; is_set_minimum = true; return minimum;}
    int setMaximum(int value) {maximum = value; is_set = true; is_set_maximum = true; return maximum;}
    const int getMinimum() const { return minimum; }
    const int getMaximum() const { return maximum; }

    friend std::ostream& operator<<(std::ostream& os, const MessageElementConstraints& obj) {
        if(obj.is_set){
            os << "\"constraints\" : {";
            if(obj.is_set_minimum){
                os << "\"minimum\":" << obj.minimum;
            }
            if(obj.is_set_minimum && obj.is_set_maximum){
                os << ", ";
            }
            if(obj.is_set_maximum){
                os << "\"maximum\":" << obj.maximum;
            }
            os << "}";
        } else {
            os << "<object not set>";
        }
        return os;
    }

private:
    bool is_set = false;
    bool is_set_minimum = false;
    bool is_set_maximum = false;
    double minimum;
    double maximum;
};

class MessageElement {
public:
    enum class MessageElementType {
        MET_UNDEFINED = 0,
        MET_INTEGER = 10,
        MET_UNSIGNED_INTEGER = 11,
        MET_DECIMAL = 20,
        MET_STRING = 30,
        MET_BOOLEAN = 40,
        MET_STRUCTURE = 100,
        MET_ROUTING = 200,
        MET_PAYLOAD = 1000
    };
    MessageElement() {}
    MessageElement(const std::string& name_, size_t bitLength_, size_t size_,
                    const MessageElementType& type_, const MessageElementConstraints& constraints_,
                    const std::vector<MessageElementDependency>& dependencies_) : 
      name(name_), bitLength(bitLength_), size(size_), type(type_), constraints(constraints_), dependencies(dependencies_) {}

    std::string setName(const std::string& name_) {name = name_; return name;}
    size_t setBitLength(size_t bitLength_) {bitLength = bitLength_; return bitLength;}
    size_t setSize(size_t size_) {
        size = size_; 
        is_array = true;
        return size;
    }
    bool setVisibility(bool is_visible_) { is_visible = is_visible_; return is_visible_; }
    bool setFlattenStructure(bool is_flatten_structure_) { is_flatten_structure = is_flatten_structure_; return is_flatten_structure; }
    std::string setArraySizeReference(const std::string arraySizeReference_) {
        arraySizeReference = arraySizeReference_;
        size = 0;
        is_array = true;
        return arraySizeReference;
    }    
    MessageElementType setType(MessageElementType type_) {type = type_; return type;}
    MessageElementConstraints setConstraints(MessageElementConstraints constraints_) {
        constraints = constraints_;
        return constraints;
    }
    std::vector<MessageElementDependency> setDependencies(const std::vector<MessageElementDependency>& dependencies_) {
        dependencies = dependencies_;
        return dependencies;
    }
    std::vector<MessageElement> setStructure(const std::vector<MessageElement>& structure_) {
        structure = structure_;
        return structure;
    }
    std::map<int, MessageElement> setRouting(const std::map<int, MessageElement>& routing_) {
        routing = routing_;
        return routing;
    }
    std::string getName() const {return name;}
    size_t getBitLength() const {return bitLength;}
    size_t getSize() const {return size;}
    bool isVisible() const {return is_visible;}
    bool isFlattenStructure() const {return is_flatten_structure;}
    std::string getArraySizeReference() const {return arraySizeReference;}
    MessageElementType getType() const {return type;}
    const MessageElementConstraints getConstraints() const {return constraints;}
    const std::vector<MessageElementDependency> getDependencies() const {return dependencies;}
    const std::vector<MessageElement> getStructure() const {return structure;}
    const std::map<int, MessageElement> getRouting() const {return routing;}

    const bool isArray() const {return is_array;} 
    const bool forceIsArray(bool is_array_) const {is_array = is_array_; return is_array;} 

    friend std::ostream& operator<<(std::ostream& os, const MessageElement& obj) {
        os << "{\"name\":\"" << obj.name << "\", \"bit_length\":" << obj.bitLength <<  "\", \"size\":" << obj.size << ", \"type\":\"" << obj.MessageElementTypeToString(obj.type) << "\"";
        if(obj.constraints.isSet()) os << "," << obj.constraints;
        if(obj.dependencies.size()>0){
            os << ", \"dependencies\":[";
            for(int i = 0; i < obj.dependencies.size(); i++){
                os << obj.dependencies[i];
                if(i < obj.dependencies.size()-1){ os << ", ";}
            }
            os << "]";
        }
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

    static MessageElementType stringToMessageElementType(const std::string& str) {
      static const std::map<std::string, MessageElementType> messageElementTypeMap = {
        {"integer", MessageElementType::MET_INTEGER},
        {"unsigned integer", MessageElementType::MET_UNSIGNED_INTEGER},
        {"decimal", MessageElementType::MET_DECIMAL},
        {"string", MessageElementType::MET_STRING},
        {"boolean", MessageElementType::MET_BOOLEAN},
        {"structure", MessageElementType::MET_STRUCTURE},
        {"routing", MessageElementType::MET_ROUTING},
        {"payload", MessageElementType::MET_PAYLOAD},
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

private:
    std::string name = "<undefined>";
    size_t bitLength = 8;
    size_t size = 1;
    mutable bool is_array = false;
    mutable bool is_visible = true;
    mutable bool is_flatten_structure = false;
    std::string arraySizeReference = "<undefined>";
    MessageElementType type;
    MessageElementConstraints constraints;
    std::vector<MessageElementDependency> dependencies;
    std::vector<MessageElement> structure;
    std::map<int, MessageElement> routing;
};

