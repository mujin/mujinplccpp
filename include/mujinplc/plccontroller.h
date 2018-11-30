#ifndef MUJINPLC_PLCCONTROLLER_H
#define MUJINPLC_PLCCONTROLLER_H

#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>

#include <mujinplc/config.h>
#include <mujinplc/plcmemory.h>

namespace mujinplc {

class MUJINPLC_API PLCControllerObserver;
class MUJINPLC_API PLCController {
public:
    PLCController(const std::shared_ptr<PLCMemory>& memory, const std::chrono::milliseconds& maxHeartbeatInterval=std::chrono::milliseconds::zero(), const std::string& heartbeatSignal="");
    virtual ~PLCController();

    // whether plc is connected
    virtual bool IsConnected() const;

    // dequeue all changes to sync this snapshot to the current plc state
    virtual void Sync();

    // wait until plc connected, IsConnected() would return true
    virtual bool WaitUntilConnected(const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

    // wait for the key to become the expected value after some change, if value is null, wait for any change of the key
    // if the key is already at the value, it waits for it to change to something else, then back
    virtual bool WaitFor(const std::string& key, const PLCValue& value, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

    // wait for multiple keys, return as soon as any one key has the expected value.
    // if the passed in expected value of a key is null, then wait for any change to that key.
    virtual bool WaitForAny(const std::map<std::string, PLCValue>& keyvalues, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

    // wait until a key is at expected value.
    // if already at such value, return immediately.
    virtual bool WaitUntil(const std::string& key, const PLCValue& value, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

    // wait until multiple keys are all at their expected value, or any one key is at its exceptional value.
    // if all keys are already satisfying the expectations, return immediately.
    // if any of the exceptional conditions is met, return immediately.
    virtual bool WaitUntilAllUnless(const std::map<std::string, PLCValue>& keyvalues, const std::map<std::string, PLCValue>& exceptions, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

private:
    void _Enqueue(const std::map<std::string, PLCValue>& keyvalues);
    bool _Dequeue(std::map<std::string, PLCValue>& keyvalues, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero(), bool timeoutOnDisconnect=true);
    void _DequeueAll();

    std::shared_ptr<PLCMemory> _memory;
    std::chrono::milliseconds _maxHeartbeatInterval;
    std::string _heartbeatSignal;

    std::chrono::time_point<std::chrono::steady_clock> _lastHeartbeat; ///< timestamp of last successful heartbeat

    std::map<std::string, PLCValue> _state; ///< no lock protection, current snapshot of the memory

    std::deque<std::map<std::string, PLCValue>> _queue; ///< incoming memory modifications, protected by _mutex
    std::condition_variable _condition; ///< incoming memory modification condition variable, protected by _mutex
    std::mutex _mutex; ///< protects _queue and _condition

    std::shared_ptr<PLCControllerObserver> _observer;

    friend class PLCControllerObserver; ///< so that _Enqueue can be called
};

}

#endif
