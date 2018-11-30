#include "mujinplc/plccontroller.h"

namespace mujinplc {

class PLCControllerObserver : public PLCMemoryObserver {
public:
    PLCControllerObserver(PLCController *controller) : _controller(controller) {
    }
    virtual ~PLCControllerObserver() {
    }

    virtual void MemoryModified(const std::map<std::string, PLCValue>& keyvalues) {
        _controller->_Enqueue(keyvalues);
    }

    PLCController *_controller;
};

}

mujinplc::PLCController::PLCController(const std::shared_ptr<mujinplc::PLCMemory>& memory, const std::chrono::milliseconds& maxHeartbeatInterval, const std::string& heartbeatSignal) : _memory(memory), _maxHeartbeatInterval(maxHeartbeatInterval), _heartbeatSignal(heartbeatSignal) {

    _observer.reset(new PLCControllerObserver(this));
    _memory->AddObserver(_observer);
}

mujinplc::PLCController::~PLCController() {
}

void mujinplc::PLCController::_Enqueue(const std::map<std::string, mujinplc::PLCValue>& keyvalues) {
    if (_heartbeatSignal == "" || keyvalues.find(_heartbeatSignal) != keyvalues.end()) {
        _lastHeartbeat = std::chrono::steady_clock::now();
    }
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.push_back(keyvalues);
    }
    _condition.notify_all();
}

bool mujinplc::PLCController::_Dequeue(std::map<std::string, mujinplc::PLCValue>& keyvalues, const std::chrono::milliseconds& timeout, bool timeoutOnDisconnect) {
    auto start = std::chrono::steady_clock::now();

    keyvalues.clear();
    while (true) {
        if (timeout.count() != 0 && timeout.count() < 0) {
            // timed out
            return false;
        }

        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_condition.wait_for(lock, std::chrono::milliseconds(50)) == std::cv_status::no_timeout) {
                keyvalues = _queue.front();
                _queue.pop_front();
                // successfully took
                return true;
            }
        }

        if (timeout.count() != 0 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) > timeout) {
            // timed out
            return false;
        }

        if (timeoutOnDisconnect && !IsConnected()) {
            // if disconnection is detected, immediately timeout
            return false;
        }
    }

    for (auto& keyvalue : keyvalues) {
        _state[keyvalue.first] = keyvalue.second;
    }
}

void mujinplc::PLCController::_DequeueAll() {
    std::map<std::string, mujinplc::PLCValue> keyvalues;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_queue.empty()) {
            for (auto& keyvalue : _queue.front()) {
                 keyvalues[keyvalue.first] = keyvalue.second;
            }
            _queue.pop_front();
        }
    }

    for (auto& keyvalue : keyvalues) {
        _state[keyvalue.first] = keyvalue.second;
    }
}

void mujinplc::PLCController::Sync() {
    _DequeueAll();
}

bool mujinplc::PLCController::IsConnected() const {
    if (_maxHeartbeatInterval.count() != 0) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _lastHeartbeat) < _maxHeartbeatInterval;
    }
    return true;
}

bool mujinplc::PLCController::WaitUntilConnected(const std::chrono::milliseconds& timeout) {
    std::map<std::string, mujinplc::PLCValue> modifications;
    std::chrono::milliseconds timeleft = timeout;
    while (!IsConnected()) {
        auto start = std::chrono::steady_clock::now();
        if (!_Dequeue(modifications, timeout, false)) {
            return false;
        }

        if (timeleft.count() != 0) {
            timeleft -= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        }
    }
    return true;
}

bool mujinplc::PLCController::WaitFor(const std::string& key, const mujinplc::PLCValue& value, const std::chrono::milliseconds& timeout) {
    std::map<std::string, mujinplc::PLCValue> keyvalues;
    keyvalues.emplace(key, value);
    return WaitForAny(keyvalues, timeout);
}

bool mujinplc::PLCController::WaitForAny(const std::map<std::string, mujinplc::PLCValue>& keyvalues, const std::chrono::milliseconds& timeout) {
    std::map<std::string, mujinplc::PLCValue> modifications;
    std::chrono::milliseconds timeleft = timeout;
    while (true) {
        auto start = std::chrono::steady_clock::now();

        if (!_Dequeue(modifications, timeleft)) {
            return false;
        }

        for (auto& modification : modifications) {
            auto it = keyvalues.find(modification.first);
            if (it != keyvalues.end()) {
                if (it->second.IsNull() || modification.second == it->second) {
                    return true;
                }
            }
        }

        if (timeleft.count() != 0) {
            timeleft -= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        }
    }
}

bool mujinplc::PLCController::WaitUntil(const std::string& key, const mujinplc::PLCValue& value, const std::chrono::milliseconds& timeout) {
    std::map<std::string, mujinplc::PLCValue> expectations, exceptions;
    expectations.emplace(key, value);
    return WaitUntilAll(expectations, exceptions, timeout);
}

bool mujinplc::PLCController::WaitUntilAll(const std::map<std::string, mujinplc::PLCValue>& expectations, const std::map<std::string, mujinplc::PLCValue>& exceptions, const std::chrono::milliseconds& timeout) {
    std::map<std::string, mujinplc::PLCValue> keyvalues;
    std::chrono::milliseconds timeleft = timeout;

    // combine dictionaries
    keyvalues.insert(expectations.begin(), expectations.end());
    keyvalues.insert(exceptions.begin(), exceptions.end());

    // always clear the queue first
    _DequeueAll();

    while (true) {
        // check if any exceptions is already met
        for (auto& keyvalue : exceptions) {
            auto it = _state.find(keyvalue.first);
            if (it != _state.end() && it->second == keyvalue.second) {
                return true;
            }
        }

        // check if all expectations are already met
        bool met = true;
        for (auto& keyvalue : expectations) {
            auto it = _state.find(keyvalue.first);
            if (it != _state.end() && it->second != keyvalue.second) {
                met = false;
                break;
            }
        }
        if (met) {
            return true;
        }

        // wait for it to change
        auto start = std::chrono::steady_clock::now();
        if (!WaitForAny(keyvalues, timeleft)) {
            return false;
        }

        if (timeleft.count() != 0) {
            timeleft -= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        }
    }
}

void mujinplc::PLCController::Set(const std::string& key, const PLCValue& value) {
    std::map<std::string, mujinplc::PLCValue> keyvalues;
    keyvalues.emplace(key, value);
    _memory->Write(keyvalues);
}


void mujinplc::PLCController::Set(const std::map<std::string, PLCValue>& keyvalues) {
    _memory->Write(keyvalues);
}


const mujinplc::PLCValue& mujinplc::PLCController::Get(const std::string& key, const mujinplc::PLCValue& defaultValue) const {
    auto it = _state.find(key);
    if (it != _state.end()) {
        return it->second;
    }
    return defaultValue;
}

const mujinplc::PLCValue& mujinplc::PLCController::SyncAndGet(const std::string& key, const mujinplc::PLCValue& defaultValue) {
    Sync();
    return Get(key, defaultValue);
}

const std::string& mujinplc::PLCController::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = _state.find(key);
    if (it != _state.end()) {
        if (it->second.IsString()) {
            return it->second.GetString();
        }
    }
    return defaultValue;
}

const std::string& mujinplc::PLCController::SyncAndGetString(const std::string& key, const std::string& defaultValue) {
    Sync();
    return GetString(key, defaultValue);
}

int mujinplc::PLCController::GetInteger(const std::string& key, int defaultValue) const {
    auto it = _state.find(key);
    if (it != _state.end()) {
        if (it->second.IsInteger()) {
            return it->second.GetInteger();
        }
    }
    return defaultValue;
}

int mujinplc::PLCController::SyncAndGetInteger(const std::string& key, int defaultValue) {
    Sync();
    return GetInteger(key, defaultValue);
}

bool mujinplc::PLCController::GetBoolean(const std::string& key, bool defaultValue) const {
    auto it = _state.find(key);
    if (it != _state.end()) {
        if (it->second.IsBoolean()) {
            return it->second.GetBoolean();
        }
    }
    return defaultValue;
}

bool mujinplc::PLCController::SyncAndGetBoolean(const std::string& key, bool defaultValue) {
    Sync();
    return GetBoolean(key, defaultValue);
}
