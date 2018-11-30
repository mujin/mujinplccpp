#ifndef MUJINPLC_PLCCONTROLLER_H
#define MUJINPLC_PLCCONTROLLER_H

#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>

#include <mujinplc/config.h>
#include <mujinplc/plcmemory.h>

namespace mujinplc {

class MUJINPLC_API PLCController : public PLCMemoryObserver {
public:
    PLCController(const std::shared_ptr<PLCMemory>& memory);
    virtual ~PLCController();

    virtual void MemoryModified(const std::map<std::string, PLCValue>& keyvalues) override;

private:
    std::shared_ptr<PLCMemory> memory;
    std::map<std::string, PLCValue> snapshot;
    std::deque<std::map<std::string, PLCValue>> queue;
    std::condition_variable condition;
    std::mutex mutex;
};

}

#endif
