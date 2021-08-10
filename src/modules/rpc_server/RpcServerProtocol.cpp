//
// Created by Brett Chaldecott on 2021/07/19.
//

#include "keto/rpc_server/RpcServerProtocol.hpp"

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

std::string RpcServerProtocol::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


// the rpc sever protocl starter
RpcServerProtocol::RpcServerProtocol(int sessionId) : sessionId(sessionId), started(false) {
    rpcReceiveQueuePtr = RpcReceiveQueuePtr(new RpcReceiveQueue(sessionId));
    rpcSendQueuePtr = RpcSendQueuePtr(new RpcSendQueue(sessionId));
}

RpcServerProtocol::~RpcServerProtocol() {

}

void RpcServerProtocol::start(const RpcSessionSocketPtr& rpcSessionSocket) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->started = true;
    rpcSendQueuePtr->start(rpcSessionSocket);
    rpcReceiveQueuePtr->start(rpcSendQueuePtr);
}

void RpcServerProtocol::preStop() {
    rpcSendQueuePtr->preStop();
    rpcReceiveQueuePtr->preStop();
}

void RpcServerProtocol::stop() {
    rpcSendQueuePtr->stop();
    rpcReceiveQueuePtr->stop();
}

void RpcServerProtocol::abort() {
    rpcSendQueuePtr->stop();
    rpcReceiveQueuePtr->stop();
}

void RpcServerProtocol::join() {
    rpcSendQueuePtr->join();
    rpcReceiveQueuePtr->join();
}

bool RpcServerProtocol::isStarted() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return started;
}

RpcReceiveQueuePtr RpcServerProtocol::getReceiveQueue() {
    return this->rpcReceiveQueuePtr;
}

RpcSendQueuePtr RpcServerProtocol::getSendQueue() {
    return this->rpcSendQueuePtr;
}


}
}
