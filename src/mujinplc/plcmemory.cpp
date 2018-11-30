#include "mujinplc/plcmemory.h"

mujinplc::PLCValue::PLCValue() : type(mujinplc::PLCValueType_Null) {
}

mujinplc::PLCValue::PLCValue(std::string value) : type(mujinplc::PLCValueType_String), stringValue(value) {
}

mujinplc::PLCValue::PLCValue(int value) : type(mujinplc::PLCValueType_Integer), integerValue(value) {
}

mujinplc::PLCValue::PLCValue(bool value) : type(mujinplc::PLCValueType_Boolean), booleanValue(value) {
}

mujinplc::PLCValue::PLCValue(const mujinplc::PLCValue& other) : type(other.type), stringValue(other.stringValue), integerValue(other.integerValue), booleanValue(other.booleanValue) {
}

mujinplc::PLCValue::~PLCValue() {
}

bool mujinplc::PLCValue::IsString() const {
    return type == mujinplc::PLCValueType_String;
}

const std::string& mujinplc::PLCValue::GetString() const {
    return stringValue;
}

void mujinplc::PLCValue::SetString(const std::string& value) {
    type = mujinplc::PLCValueType_String;
    stringValue = value;
}

bool mujinplc::PLCValue::IsBoolean() const {
    return type == mujinplc::PLCValueType_Boolean;
}

bool mujinplc::PLCValue::GetBoolean() const {
    return booleanValue;
}

void mujinplc::PLCValue::SetBoolean(bool value) {
    type = mujinplc::PLCValueType_Boolean;
    booleanValue = value;
}

bool mujinplc::PLCValue::IsInteger() const {
    return type == mujinplc::PLCValueType_Integer;
}

int mujinplc::PLCValue::GetInteger() const {
    return integerValue;
}

void mujinplc::PLCValue::SetInteger(int value) {
    type = mujinplc::PLCValueType_Integer;
    integerValue = value;
}

bool mujinplc::PLCValue::IsNull() const {
    return type == mujinplc::PLCValueType_Null;
}

void mujinplc::PLCValue::SetNull() {
    type = mujinplc::PLCValueType_Null;
}

bool mujinplc::operator==(const mujinplc::PLCValue& lhs, const mujinplc::PLCValue& rhs) {
    if (lhs.IsString()) {
        return rhs.IsString() && lhs.GetString() == rhs.GetString();
    }
    if (lhs.IsBoolean()) {
        return rhs.IsBoolean() && lhs.GetBoolean() == rhs.GetBoolean();
    }
    if (lhs.IsInteger()) {
        return rhs.IsInteger() && lhs.GetInteger() == rhs.GetInteger();
    }
    if (lhs.IsNull()) {
        return rhs.IsNull();
    }
    return false;
}

bool mujinplc::operator!=(const mujinplc::PLCValue& lhs, const mujinplc::PLCValue& rhs) {
    return !(lhs == rhs);
}

mujinplc::PLCMemory::PLCMemory() {
}

mujinplc::PLCMemory::~PLCMemory() {
}

void mujinplc::PLCMemory::Read(const std::vector<std::string> &keys, std::map<std::string, mujinplc::PLCValue> &keyvalues) {
    keyvalues.clear();

    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& key : keys) {
            auto it = entries.find(key);
            if (it != entries.end()) {
                keyvalues.emplace(it->first, it->second);
            }
        }
    }
}


void mujinplc::PLCMemory::Write(const std::map<std::string, mujinplc::PLCValue> &keyvalues) {
    std::map<std::string, mujinplc::PLCValue> modifications;
    std::vector<std::weak_ptr<PLCMemoryObserver>> observersCopy;

    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& keyvalue : keyvalues) {
            auto it = entries.find(keyvalue.first);
            if (it != entries.end()) {
                if (it->second != keyvalue.second) {
                    it->second = keyvalue.second;
                    modifications.emplace(keyvalue.first, keyvalue.second);
                }
            } else {
                entries.emplace(keyvalue.first, keyvalue.second);
                modifications.emplace(keyvalue.first, keyvalue.second);
            }
        }

        // copy under lock
        if (modifications.size() > 0) {
            observersCopy =  observers;
        }
    }

    for (auto& observerWeak : observersCopy) {
        if (auto observer = observerWeak.lock()) {
            observer->MemoryModified(modifications);
        }
    }
}

void mujinplc::PLCMemory::AddObserver(const std::shared_ptr<PLCMemoryObserver>& observer) {
    std::map<std::string, mujinplc::PLCValue> entriesCopy;
    {
        std::lock_guard<std::mutex> lock(mutex);
        observers.push_back(observer);
        entriesCopy = entries;
    }
    if (entriesCopy.size() > 0) {
        observer->MemoryModified(entriesCopy);
    }
}
