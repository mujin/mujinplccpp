#include <iostream>
#include <mujinplc/mujinplc.h>

int main() {
    std::shared_ptr<mujinplc::PLCMemory> memory(new mujinplc::PLCMemory());
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
