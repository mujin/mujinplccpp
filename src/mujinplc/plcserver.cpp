#include "mujinplc/plcserver.h"

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
        if (ctxown) {
            zmq_ctx_destroy(ctxown);
        }
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
        }

        try {

        } catch (const zmq::Error& e) {
            socket.release();
        }
    }
}