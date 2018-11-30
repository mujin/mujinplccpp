#include "mujinplc/plccontroller.h"

mujinplc::PLCController::PLCController(const std::shared_ptr<mujinplc::PLCMemory>& memory) : memory(memory) {
}

mujinplc::PLCController::~PLCController() {
}

void mujinplc::PLCController::MemoryModified(const std::map<std::string, mujinplc::PLCValue>& keyvalues) {
    {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push_back(keyvalues);
    }
    condition.notify_all();
}
