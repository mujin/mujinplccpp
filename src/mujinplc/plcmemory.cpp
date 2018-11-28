#include "mujinplc/plcmemory.h"

using namespace mujinplc;

PLCValue::PLCValue(std::string value) : type(PLCValueType_String), stringValue(value) {
}

PLCValue::PLCValue(int value) : type(PLCValueType_Integer), integerValue(value) {
}

PLCValue::PLCValue(bool value) : type(PLCValueType_Boolean), booleanValue(value) {
}

PLCValue::PLCValue(const PLCValue& other) : type(other.type), stringValue(other.stringValue), integerValue(other.integerValue), booleanValue(other.booleanValue) {
}

PLCValue::PLCValue(const rapidjson::Value &value) {
    if (value.IsString()) {
        type = PLCValueType_String;
        stringValue = value.GetString();
    }
    else if (value.IsBool()) {
        type = PLCValueType_Boolean;
        booleanValue = value.GetBool();
    }
    else if (value.IsInt()) {
        type = PLCValueType_Integer;
        integerValue = value.GetInt();
    }
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
    }
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
    std::lock_guard<std::mutex> lock(mutex);

    for (auto it = input.MemberBegin(); it != input.MemberEnd(); it++) {
        std::string key = it->name.GetString();
        entries.emplace(key, it->value);
    }
}
