#ifndef MUJINPLC_PLCLOGIC_H
#define MUJINPLC_PLCLOGIC_H

#include <chrono>
#include <memory>

#include <mujinplc/config.h>
#include <mujinplc/plccontroller.h>

namespace mujinplc {

class MUJINPLC_API PLCLogic {

public:
    PLCLogic(const std::shared_ptr<PLCController>& controller);
    virtual ~PLCLogic();

    virtual bool WaitUntilConnected(const std::chrono::milliseconds& timeout=std::chrono::milliseconds::zero());

private:
    std::shared_ptr<PLCController> _controller;
};

}

#endif
