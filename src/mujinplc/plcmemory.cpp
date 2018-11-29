#include "mujinplc/plcmemory.h"

// #include <iostream> // TODO: temporary

using namespace mujinplc;

PLCValue::PLCValue(std::string value) : type(PLCValueType_String), stringValue(value) {
}

PLCValue::PLCValue(int value) : type(PLCValueType_Integer), integerValue(value) {
}

PLCValue::PLCValue(bool value) : type(PLCValueType_Boolean), booleanValue(value) {
}

PLCValue::PLCValue(const PLCValue& other) : type(other.type), stringValue(other.stringValue), integerValue(other.integerValue), booleanValue(other.booleanValue) {
}

PLCValue::PLCValue(const rapidjson::Value &value) : type(PLCValueType_Null) {
    Write(value);
}

PLCValue::~PLCValue() {
}

void PLCValue::Read(rapidjson::Value &output, rapidjson::Document::AllocatorType &allocator) {
    switch (type) {
    case PLCValueType_String:
        output.SetString(stringValue.c_str(), allocator);
        break;
    case PLCValueType_Integer:
        output.Set(integerValue, allocator);
        break;
    case PLCValueType_Boolean:
        output.Set(booleanValue, allocator);
        break;
    default:
        output.SetNull();
        break;
    }
}

bool PLCValue::Write(const rapidjson::Value &value) {
    if (value.IsString()) {
        if (type == PLCValueType_String && stringValue == value.GetString()) {
            return false;
        }
        type = PLCValueType_String;
        stringValue = value.GetString();
        return true;
    }

    if (value.IsBool()) {
        if (type == PLCValueType_Boolean && booleanValue == value.GetBool()) {
            return false;
        }
        type = PLCValueType_Boolean;
        booleanValue = value.GetBool();
        return true;
    }


    if (value.IsInt()) {
        if (type == PLCValueType_Integer && integerValue == value.GetInt()) {
            return false;
        }
        type = PLCValueType_Integer;
        integerValue = value.GetInt();
        return true;
    }
    
    if (type == PLCValueType_Null) {
        return false;
    }
    type = PLCValueType_Null;
    return true;
}

PLCMemory::PLCMemory() {
}

PLCMemory::~PLCMemory() {
}

void PLCMemory::Read(const std::vector<std::string> &keys, rapidjson::Value &output, rapidjson::Document::AllocatorType &allocator) {
    rapidjson::Value key, value;

    std::lock_guard<std::mutex> lock(mutex);

    output.SetObject();
    for (auto it = keys.begin(); it != keys.end(); it++) {
        auto it2 = entries.find(*it);
        if (it2 != entries.end()) {
            key.SetString(it2->first.c_str(), allocator);
            it2->second.Read(value, allocator);
            output.AddMember(key, value, allocator);
        }
    }
}

void PLCMemory::Write(const rapidjson::Value &input) {
    std::map<std::string, PLCValue> modifications;

    {
        std::lock_guard<std::mutex> lock(mutex);

        for (auto it = input.MemberBegin(); it != input.MemberEnd(); it++) {
            std::string key = it->name.GetString();
            auto it2 = entries.find(key);
            if (it2 != entries.end()) {
                if (it2->second.Write(it->value)) {
                    modifications.emplace(key, it->value);
                }
            } else {
                entries.emplace(key, it->value);
                modifications.emplace(key, it->value);
            }
        }
    }

    if (modifications.size() > 0) {
        // std::cout << "Memory changed: " << modifications.size() << std::endl;
    }
}
