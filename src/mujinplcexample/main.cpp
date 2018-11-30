#include <iostream>
#include <mujinplc/mujinplc.h>

class MemoryLogger : public mujinplc::PLCMemoryObserver {
public:
    MemoryLogger() {
    }
    virtual ~MemoryLogger() {
    }

    virtual void MemoryModified(const std::map<std::string, mujinplc::PLCValue>& keyvalues) {
        std::cout << "Memory modified: ";
        for (auto& keyvalue : keyvalues) {
            if (keyvalue.second.IsString()) {
                std::cout << keyvalue.first << " = \"" << keyvalue.second.GetString() << "\", ";
            }
            else if (keyvalue.second.IsInteger()) {
                std::cout << keyvalue.first << " = " << keyvalue.second.GetInteger() << ", ";
            }
            else if (keyvalue.second.IsBoolean()) {
                std::cout << keyvalue.first << " = " << (keyvalue.second.GetBoolean() ? "true" : "false") << ", ";
            }
            else {
                std::cout << keyvalue.first << " = null, ";
            }
        }
        std::cout << "total " << keyvalues.size() << " entries." << std::endl;
    }
};

int main() {
    std::shared_ptr<mujinplc::PLCMemory> memory(new mujinplc::PLCMemory());
    std::shared_ptr<MemoryLogger> logger(new MemoryLogger());
    memory->AddObserver(logger);

    std::shared_ptr<mujinplc::PLCServer> server(new mujinplc::PLCServer(memory, NULL, "tcp://*:5555"));
    server->Start();

    std::cout << "Server started." << std::endl;
    do 
    {
        std::cout << std::endl << "Press ENTER key to exit ...";
    } while (std::cin.get() != '\n');

    std::cout << "Server stopping." << std::endl;

    server->Stop();

    std::cout << "Server stopped." << std::endl;
}
