#include "mujinplc/plcserver.h"

#include <vector>
#include <zmq.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace mujinplc {

class ZMQError : public std::exception {
public:
    ZMQError();
    virtual ~ZMQError();

    virtual const char* what() const noexcept override;

private:
    int _errno;
};

class ZMQServerSocket {
public:
    ZMQServerSocket(void* ctxin, const std::string& endpoint);
    virtual ~ZMQServerSocket();

    bool Poll(long timeout);
    void Receive(rapidjson::Document& doc);
    void Send(const rapidjson::Value& value);

private:
    void* _ctx;
    void* _socket;
    zmq_msg_t _message;
};

}

mujinplc::ZMQError::ZMQError() : _errno(zmq_errno()) {
}

mujinplc::ZMQError::~ZMQError() {
}

const char* mujinplc::ZMQError::what() const noexcept {
    return zmq_strerror(_errno);
}

mujinplc::ZMQServerSocket::ZMQServerSocket(void* ctx, const std::string& endpoint) : _ctx(NULL), _socket(NULL) {
    if (!ctx) {
        _ctx = zmq_ctx_new();
        if (_ctx == NULL) {
            throw mujinplc::ZMQError();
        }
        ctx = _ctx;
    }

    _socket = zmq_socket(ctx, ZMQ_REP);
    if (_socket == NULL) {
        throw mujinplc::ZMQError();
    }

    int linger = 100;
    if (zmq_setsockopt(_socket, ZMQ_LINGER, &linger, sizeof(linger))) {
        throw mujinplc::ZMQError();
    }

    int sndhwm = 2;
    if (zmq_setsockopt(_socket, ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm))) {
        throw mujinplc::ZMQError();
    }

    if (zmq_bind(_socket, endpoint.c_str())) {
        throw mujinplc::ZMQError();
    }
}

mujinplc::ZMQServerSocket::~ZMQServerSocket() {
    zmq_msg_close(&_message);
    if (_socket) {
        zmq_close(_socket);
        _socket = NULL;
    }
    if (_ctx) {
        zmq_ctx_destroy(_ctx);
        _ctx = NULL;
    }
}

bool mujinplc::ZMQServerSocket::Poll(long timeout) {
    zmq_pollitem_t item;
    item.socket = _socket;
    item.events = ZMQ_POLLIN;

    int rc = zmq_poll(&item, 1, timeout);
    if (rc < 0) {
        throw mujinplc::ZMQError();
    }

    return rc > 0;
}

void mujinplc::ZMQServerSocket::Receive(rapidjson::Document& doc) {
    zmq_msg_close(&_message);
    if (zmq_msg_init(&_message)) {
        throw mujinplc::ZMQError();
    }

    int nbytes = zmq_msg_recv(&_message, _socket, ZMQ_NOBLOCK);
    if (nbytes < 0) {
        throw mujinplc::ZMQError();
    }

    std::string data((char*)zmq_msg_data(&_message), zmq_msg_size(&_message));
    doc.Parse<rapidjson::kParseFullPrecisionFlag>(data.c_str());
}

void mujinplc::ZMQServerSocket::Send(const rapidjson::Value& value) {
    rapidjson::StringBuffer stringbuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(stringbuffer);
    value.Accept(writer);

    int nbytes = zmq_send(_socket, stringbuffer.GetString(), stringbuffer.GetSize(), ZMQ_NOBLOCK);
    if (nbytes < 0) {
        throw mujinplc::ZMQError();
    }
}

mujinplc::PLCServer::PLCServer(const std::shared_ptr<mujinplc::PLCMemory>& memory, void* ctx, const std::string& endpoint) : _shutdown(true), _memory(memory), _ctx(ctx), _endpoint(endpoint) {
}

mujinplc::PLCServer::~PLCServer() {
    _shutdown = true;
}

bool mujinplc::PLCServer::IsRunning() const {
    return !_shutdown || _thread.joinable();
}

void mujinplc::PLCServer::Start() {
    Stop();

    _shutdown = false;
    _thread = std::thread(&mujinplc::PLCServer::_RunThread, this);
}

void mujinplc::PLCServer::SetStop() {
    _shutdown = true;
}

void mujinplc::PLCServer::Stop() {
    SetStop();
    if (_thread.joinable()) {
        _thread.join();
    }
}

void mujinplc::PLCServer::_RunThread() {
    std::unique_ptr<mujinplc::ZMQServerSocket> socket;

    while (!_shutdown) {
        if (!socket) {
            socket.reset(new mujinplc::ZMQServerSocket(_ctx, _endpoint));
        }

        try {
            if (!socket->Poll(50)) {
                continue;
            }

            // something on the socket, receive json
            rapidjson::Document request, response;
            response.SetObject();
            socket->Receive(request);

            // read command, expects a list of keys
            if (request.IsObject() &&
                request.HasMember("command") &&
                request["command"].IsString() &&
                request["command"].GetString() == std::string("read") &&
                request.HasMember("keys") &&
                request["keys"].IsArray()) {

                std::vector<std::string> keys;
                std::map<std::string, mujinplc::PLCValue> keyvalues;
                for (auto& key : request["keys"].GetArray()) {
                    if (key.IsString()) {
                        keys.push_back(key.GetString());
                    }
                }
                _memory->Read(keys, keyvalues);

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
            // write command, expects a dict of keyvalues
            else if (request.IsObject() && 
                request.HasMember("command") &&
                request["command"].IsString() &&
                request["command"].GetString() == std::string("write") &&
                request.HasMember("keyvalues") &&
                request["keyvalues"].IsObject()) {

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
                _memory->Write(keyvalues);
            }

            socket->Send(response);
        } catch (const mujinplc::ZMQError& e) {
            socket.release();
            // std::cout << "Error caught: " << e.what() << std::endl;
        }
    }
}
