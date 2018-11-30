#include "mujinplc/plclogic.h"

mujinplc::PLCLogic::PLCLogic(const std::shared_ptr<mujinplc::PLCController>& controller) : _controller(controller) {
}

mujinplc::PLCLogic::~PLCLogic() {
}

bool mujinplc::PLCLogic::WaitUntilConnected(const std::chrono::milliseconds& timeout) {
	return _controller->WaitUntilConnected(timeout);
}
