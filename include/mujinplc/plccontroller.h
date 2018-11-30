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

    virtual bool IsConnected() const;

    virtual void Sync();

    virtual bool WaitUntilConnected(const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

    virtual bool WaitFor(const std::string& key, const PLCValue& value, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());
    virtual bool WaitForAny(const std::map<std::string, PLCValue>& keyvalues, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

private:
    void _Enqueue(const std::map<std::string, PLCValue>& keyvalues);
    bool _Dequeue(std::map<std::string, PLCValue>& keyvalues, const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero(), bool timeoutOnDisconnect=true);
    void _DequeueAll();

    std::shared_ptr<PLCMemory> _memory;
    std::chrono::milliseconds _maxHeartbeatInterval;
    std::string _heartbeatSignal;
    std::chrono::time_point<std::chrono::steady_clock> _lastHeartbeat;
    std::map<std::string, PLCValue> _state;
    std::deque<std::map<std::string, PLCValue>> _queue;
    std::condition_variable _condition;
    std::mutex _mutex;
    std::shared_ptr<PLCControllerObserver> _observer;

    friend class PLCControllerObserver;
};

}

#endif
