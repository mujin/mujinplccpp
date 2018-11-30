#ifndef MUJINPLC_PLCMEMORY_H
#define MUJINPLC_PLCMEMORY_H

#include <map>
#include <vector>
#include <mutex>
#include <memory>

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
    const std::string& GetString() const;
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
    PLCValueType _type;

    std::string _stringValue;
    int _integerValue;
    bool _booleanValue;
};

MUJINPLC_API bool operator==(const PLCValue& lhs, const PLCValue& rhs);
MUJINPLC_API bool operator!=(const PLCValue& lhs, const PLCValue& rhs);

class MUJINPLC_API PLCMemoryObserver {
public:
    virtual ~PLCMemoryObserver() = default;
    virtual void MemoryModified(const std::map<std::string, PLCValue>& keyvalues) = 0;
};

class MUJINPLC_API PLCMemory {
public:
    PLCMemory();
    virtual ~PLCMemory();

    void Read(const std::vector<std::string> &keys, std::map<std::string, PLCValue> &keyvalues);
    void Write(const std::map<std::string, PLCValue> &keyvalues);

    void AddObserver(const std::shared_ptr<PLCMemoryObserver>& observer);

private:
    std::map<std::string, PLCValue> _entries;
    std::mutex _mutex;
    std::vector<std::weak_ptr<PLCMemoryObserver>> _observers;
};

}

#endif
