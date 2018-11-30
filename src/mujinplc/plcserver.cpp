#include "mujinplc/plcserver.h"

#include <vector>
#include <iostream> // TODO: temporary
#include <zmq.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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

mujinplc::PLCServer::PLCServer(const std::shared_ptr<mujinplc::PLCMemory>& memory, void* ctx, const std::string& endpoint) : shutdown(false), memory(memory), ctx(ctx), endpoint(endpoint) {
}

mujinplc::PLCServer::~PLCServer() {
    shutdown = true;
}

void mujinplc::PLCServer::Start() {
    Stop();

    shutdown = false;
    thread = std::thread(&mujinplc::PLCServer::_RunThread, this);
}

void mujinplc::PLCServer::Stop() {
    shutdown = true;
    if (thread.joinable()) {
        thread.join();
    }
}

void mujinplc::PLCServer::_RunThread() {
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
                response.SetObject();

                socket->Receive(request);

                if (request.IsObject() && request.HasMember("command") && request["command"].IsString()) {
                    std::string command = request["command"].GetString();
                    if (command == "read") {
                        if (request.HasMember("keys") && request["keys"].IsArray()) {
                            std::vector<std::string> keys;
                            std::map<std::string, mujinplc::PLCValue> keyvalues;
                            for (auto& key : request["keys"].GetArray()) {
                                if (key.IsString()) {
                                    keys.push_back(key.GetString());
                                }
                            }
                            memory->Read(keys, keyvalues);

                            {
                                rapidjson::Value key, value, values;
                                values.SetObject();
                                for (auto& keyvalue : keyvalues) {
                                    key.SetString(keyvalue.first.c_str(), response.GetAllocator());
                                    if (keyvalue.second.IsString()) {
                                        value.SetString(keyvalue.second.GetString().c_str(), response.GetAllocator());
                                    }
                                    else if (keyvalue.second.IsInteger()) {
                                        value.SetInt(keyvalue.second.GetInteger());
                                    }
                                    else if (keyvalue.second.IsBoolean()) {
                                        value.SetBool(keyvalue.second.GetBoolean());
                                    }
                                    else {
                                        value.SetNull();
                                    }
                                    values.AddMember(key, value, response.GetAllocator());
                                }
                                key.SetString("keyvalues", response.GetAllocator());
                                response.AddMember(key, values, response.GetAllocator());
                            }
                        }
                    }
                    else if (command == "write") {
                        if (request.HasMember("keyvalues") && request["keyvalues"].IsObject()) {
                            std::map<std::string, mujinplc::PLCValue> keyvalues;
                            for (auto& keyvalue : request["keyvalues"].GetObject()) {
                                if (!keyvalue.name.IsString()) {
                                    continue;
                                }
                                if (keyvalue.value.IsString()) {
                                    keyvalues.emplace(keyvalue.name.GetString(), std::string(keyvalue.value.GetString()));
                                }
                                else if (keyvalue.value.IsBool()) {
                                    keyvalues.emplace(keyvalue.name.GetString(), bool(keyvalue.value.GetBool()));
                                }
                                else if (keyvalue.value.IsInt()) {
                                    keyvalues.emplace(keyvalue.name.GetString(), int(keyvalue.value.GetInt()));
                                }
                                else {
                                    keyvalues.emplace(keyvalue.name.GetString(), mujinplc::PLCValue());
                                }
                            }
                            memory->Write(keyvalues);
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
