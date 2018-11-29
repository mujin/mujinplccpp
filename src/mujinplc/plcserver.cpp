#include "mujinplc/plcserver.h"

#include <vector>
#include <iostream> // TODO: temporary
#include <zmq.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace mujinplc;

namespace zmq {

class Error : public std::exception {
public:
    Error() : errnum(zmq_errno()) {}
    virtual ~Error() {}
    virtual const char* what() const noexcept { return zmq_strerror(errno); }

private:
    int errnum;
};

class Message {
public:
    Message() {
        if (zmq_msg_init(&message)) {
            throw Error();
        }
    }
    virtual ~Message() {
        zmq_msg_close(&message);
    }

    zmq_msg_t message;
};

class ServerSocket {
public:
    ServerSocket(void* ctxin, const std::string& endpoint) : ctx(ctxin), socket(NULL) {
        if (!ctx) {
            ctxown = zmq_ctx_new();
            if (ctxown == NULL) {
                throw Error();
            }
            ctx = ctxown;
        }

        socket = zmq_socket(ctx, ZMQ_REP);
        if (socket == NULL) {
            throw Error();
        }

        if (zmq_bind(socket, endpoint.c_str())) {
            throw Error();
        }
    }
    virtual ~ServerSocket() {
        if (socket) {
            zmq_close(socket);
            socket = NULL;
        }
        if (ctxown) {
            zmq_ctx_destroy(ctxown);
            ctxown = NULL;
        }
    }

    bool Poll(long timeout) {
        zmq_pollitem_t item;
        item.socket = socket;
        item.events = ZMQ_POLLIN;

        int rc = zmq_poll(&item, 1, timeout);
        if (rc < 0) {
            throw Error();
        }

        return rc > 0;
    }

    void Receive(rapidjson::Document& doc) {
        Message message;

        int nbytes = zmq_msg_recv(&message.message, socket, ZMQ_NOBLOCK);
        if (nbytes < 0) {
            throw Error();
        }

        std::string data((char*)zmq_msg_data(&message.message), zmq_msg_size(&message.message));
        doc.Parse<rapidjson::kParseFullPrecisionFlag>(data.c_str());
    }

    void Send(const rapidjson::Value& value) {
        rapidjson::StringBuffer stringbuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(stringbuffer);
        value.Accept(writer);

        int nbytes = zmq_send(socket, stringbuffer.GetString(), stringbuffer.GetSize(), ZMQ_NOBLOCK);
        if (nbytes < 0) {
            throw Error();
        }
    }

private:
    void* ctx;
    void* ctxown;
    void* socket;
};
}

PLCServer::PLCServer(const std::shared_ptr<PLCMemory>& memory, void* ctx, const std::string& endpoint) : shutdown(false), memory(memory), ctx(ctx), endpoint(endpoint) {
}

PLCServer::~PLCServer() {
    shutdown = true;
}

void PLCServer::Start() {
    Stop();

    shutdown = false;
    thread = std::thread(&PLCServer::_RunThread, this);
}

void PLCServer::Stop() {
    shutdown = true;
    if (thread.joinable()) {
        thread.join();
    }
}

void PLCServer::_RunThread() {
    std::unique_ptr<zmq::ServerSocket> socket;

    while (!shutdown) {
        if (!socket) {
            socket.reset(new zmq::ServerSocket(ctx, endpoint));
            std::cout << "New socket created: " << endpoint << std::endl;
        }

        try {
            if (socket->Poll(50)) {
                // something on the socket
                rapidjson::Document request, response;
                rapidjson::Value key, keyvalues;
                response.SetObject();

                socket->Receive(request);

                if (request.IsObject() && request.HasMember("command") && request["command"].IsString()) {
                    std::string command = request["command"].GetString();
                    if (command == "read") {
                        if (request.HasMember("keys") && request["keys"].IsArray()) {
                            std::vector<std::string> keys;
                            for (auto it = request["keys"].Begin(); it != request["keys"].End(); ++it) {
                                if (it->IsString()) {
                                    keys.push_back(it->GetString());
                                }
                            }
                            memory->Read(keys, keyvalues, response.GetAllocator());
                            key.SetString("keyvalues", response.GetAllocator());
                            response.AddMember(key, keyvalues, response.GetAllocator());
                        }
                    }
                    else if (command == "write") {
                        if (request.HasMember("keyvalues") && request["keyvalues"].IsObject()) {
                            memory->Write(request["keyvalues"]);
                        }
                    }
                    std::cout << "Received command " << command << " on the socket." << std::endl;
                }

                // TODO: process command
                socket->Send(response);
            }
        } catch (const zmq::Error& e) {
            socket.release();
            std::cout << "Error caught: " << e.what() << std::endl;
        }
    }

    std::cout << "Thread stopping." << std::endl;
}