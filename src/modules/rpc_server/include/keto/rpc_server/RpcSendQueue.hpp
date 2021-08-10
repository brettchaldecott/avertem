//
// Created by Brett Chaldecott on 2021/07/24.
//

#ifndef KETO_RPCSENDQUEUE_HPP
#define KETO_RPCSENDQUEUE_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "keto/common/MetaInfo.hpp"
#include "keto/crypto/Containers.hpp"

#include "keto/event/Event.hpp"

#include "keto/rpc_server/RpcSessionSocket.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_server {

class RpcSendQueueEntry {
public:
    RpcSendQueueEntry(const std::string& command, const std::string& payload) :
            command(command),payload(payload){}

    RpcSendQueueEntry(const RpcSendQueueEntry& orig) = delete;
    virtual ~RpcSendQueueEntry() {}

    std::string getCommand() {return this->command;}
    std::string getPayload() {return this->payload;}

private:
    std::string command;
    std::string payload;
};
typedef std::shared_ptr<RpcSendQueueEntry> RpcSendQueueEntryPtr;


class RpcSendQueue;
typedef std::shared_ptr<RpcSendQueue> RpcSendQueuePtr;

class RpcSendQueue {
public:

    // the queue processor
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RpcSendQueue(int sessionId);
    RpcSendQueue(const RpcSendQueue& orig) = delete;
    virtual ~RpcSendQueue();

    void start(const RpcSessionSocketPtr& rpcSessionSocketPtr);
    void preStop();
    void stop();
    void abort();
    void join();

    void pushEntry(const std::string& command, const std::string& payload);
    void releaseEntry();

private:
    int sessionId;
    bool active;
    bool closed;
    bool aborted;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    RpcSessionSocketPtr rpcSessionSocketPtr;
    std::shared_ptr<std::thread> queueThreadPtr;
    std::deque<RpcSendQueueEntryPtr> sendQueue;
    RpcSendQueueEntryPtr activeEntry;

    void run();
    RpcSendQueueEntryPtr peekEntry();
    void _pushEntry(const std::string& command, const std::string& payload);
    void processEntry(const RpcSendQueueEntryPtr& entry);


};

}
}

#endif //KETO_RPCSENDQUEUE_HPP
