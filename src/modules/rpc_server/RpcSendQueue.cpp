//
// Created by Brett Chaldecott on 2021/07/24.
//

#include "keto/rpc_server/RpcSendQueue.hpp"
#include "keto/rpc_server/Exception.hpp"
#include "keto/rpc_server/RpcSessionManager.hpp"

#include "keto/common/Log.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/rpc_server/Constants.hpp"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

std::string RpcSendQueue::getSourceVersion() {

}

RpcSendQueue::RpcSendQueue(int sessionId) : sessionId(sessionId), active(true), closed(false), aborted(false) {

}

RpcSendQueue::~RpcSendQueue() {

}

void RpcSendQueue::start(const RpcSessionSocketPtr& rpcSessionSocketPtr) {
    this->rpcSessionSocketPtr = rpcSessionSocketPtr;
    queueThreadPtr = std::shared_ptr<std::thread>(new std::thread(
            [this]
            {
                this->run();
            }));
}

void RpcSendQueue::preStop() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!this->active) {
        return;
    }
    _pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSE, keto::server_common::Constants::RPC_COMMANDS::CLOSE);
    this->active = false;
    this->stateCondition.notify_all();
}

void RpcSendQueue::stop() {

}

void RpcSendQueue::abort() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (aborted) {
        return;
    }
    this->aborted = true;
    this->stateCondition.notify_all();

}


void RpcSendQueue::join() {
    this->queueThreadPtr->join();
}

void RpcSendQueue::pushEntry(const std::string& command, const std::string& payload) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!active || aborted) {
        return;
    }
    _pushEntry(command,payload);
}

void RpcSendQueue::releaseEntry() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    //KETO_LOG_INFO << "[" << sessionId << "] release an entry";
    // release the active entry
    activeEntry.reset();
    if (sendQueue.empty()) {
        return;
    }
    this->stateCondition.notify_all();
}

void RpcSendQueue::run() {
    RpcSendQueueEntryPtr rpcSendQueueEntryPtr;
    while(rpcSendQueueEntryPtr = peekEntry()) {
        processEntry(rpcSendQueueEntryPtr);
    }
}

RpcSendQueueEntryPtr RpcSendQueue::peekEntry() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    //KETO_LOG_INFO << "[" << sessionId << "] wait for entries";
    while((activeEntry || sendQueue.empty()) && !aborted) {
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_SERVER_QUEUE_DELAY));
    }
    if (aborted) {
        return RpcSendQueueEntryPtr();
    }
    activeEntry = sendQueue.front();
    sendQueue.pop_front();
    //KETO_LOG_INFO << "[" << sessionId << "] peek an entry";
    return activeEntry;
}

void RpcSendQueue::_pushEntry(const std::string& command, const std::string& payload) {
    //KETO_LOG_INFO << "[" << sessionId << "] push an entry";
    RpcSendQueueEntryPtr rpcSendQueueEntryPtr(new RpcSendQueueEntry(command,payload));
    sendQueue.push_back(rpcSendQueueEntryPtr);
    this->stateCondition.notify_all();
}

void RpcSendQueue::processEntry(const RpcSendQueueEntryPtr& entry) {
    //KETO_LOG_INFO << "[" << sessionId << "] process an entry";
    std::stringstream ss;
    ss << entry->getCommand() << " " << entry->getPayload();
    this->rpcSessionSocketPtr->send(ss.str());
}


}
}
