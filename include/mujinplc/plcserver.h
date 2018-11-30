#ifndef MUJINPLC_PLCSERVER_H
#define MUJINPLC_PLCSERVER_H

#include <thread>
#include <mujinplc/config.h>
#include <mujinplc/plcmemory.h>

namespace mujinplc {

class MUJINPLC_API PLCServer {
public:
    PLCServer(const std::shared_ptr<PLCMemory>& memory, void* ctx, const std::string& endpoint);
    virtual ~PLCServer();

    bool IsRunning() const;
    void Start();
    void SetStop();
    void Stop();

private:
    void _RunThread();

    bool _shutdown;
    std::thread _thread;
    std::shared_ptr<PLCMemory> _memory;
    void *_ctx;
    std::string _endpoint;
};

}

#endif
