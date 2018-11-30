#ifndef MUJINPLC_PLCMEMORY_H
#define MUJINPLC_PLCMEMORY_H

#include <map>
#include <vector>
#include <mutex>
#include <mujinplc/config.h>

namespace mujinplc
{

enum MUJINPLC_API PLCValueType {
    PLCValueType_Null,
    PLCValueType_String,
    PLCValueType_Boolean,
    PLCValueType_Integer,
};

class MUJINPLC_API PLCValue {
public:
    PLCValue();
    PLCValue(std::string value);
    PLCValue(int value);
    PLCValue(bool value);
    PLCValue(const PLCValue& other);
    virtual ~PLCValue();

    bool IsString() const;
    std::string GetString() const;
    void SetString(const std::string& value);

    bool IsBoolean() const;
    bool GetBoolean() const;
    void SetBoolean(bool value);

    bool IsInteger() const;
    int GetInteger() const;
    void SetInteger(int value);

    bool IsNull() const;
    void SetNull();

private:
    PLCValueType type;

    std::string stringValue;
    int integerValue;
    bool booleanValue;
};

MUJINPLC_API bool operator==(const PLCValue& lhs, const PLCValue& rhs);
MUJINPLC_API bool operator!=(const PLCValue& lhs, const PLCValue& rhs);

class MUJINPLC_API PLCMemory {
public:
    PLCMemory();
    virtual ~PLCMemory();

    void Read(const std::vector<std::string> &keys, std::map<std::string, PLCValue> &keyvalues);
    void Write(const std::map<std::string, PLCValue> &keyvalues);

private:
    std::map<std::string, PLCValue> entries;
    std::mutex mutex;
};

}

#endif
