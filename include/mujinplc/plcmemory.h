#ifndef MUJINPLC_PLCMEMORY_H
#define MUJINPLC_PLCMEMORY_H

#include <map>
#include <vector>
#include <mutex>
#include <rapidjson/document.h>

namespace mujinplc
{

enum PLCValueType {
    PLCValueType_String,
    PLCValueType_Boolean,
    PLCValueType_Integer,
};

class PLCValue {
public:
    PLCValue(std::string value);
    PLCValue(int value);
    PLCValue(bool value);
    PLCValue(const rapidjson::Value &value);
    PLCValue(const PLCValue& other);
    virtual ~PLCValue();

    // reads out the value as rapidjson::Value
    void Read(rapidjson::Value &output, rapidjson::Document::AllocatorType &allocator);

private:
    PLCValueType type;

    std::string stringValue;
    int integerValue;
    bool booleanValue;
};

class PLCMemory {
public:
    PLCMemory();
    virtual ~PLCMemory();

    // reads keys and output dictionary in rapidjson::Value
    void Read(const std::vector<std::string> &keys, rapidjson::Value &output, rapidjson::Document::AllocatorType &allocator);

    // writes memory from given dictionary in rapidjson::Value
    void Write(const rapidjson::Value &input);

private:
    std::map<std::string, PLCValue> entries;
    std::mutex mutex;
};

}

#endif