//
// Created by Brett Chaldecott on 2021/08/13.
//

#include "keto/rpc_client/RpcClientProtocol.hpp"

#include <botan/hex.h>

#include "keto/rpc_client/RpcReceiveQueue.hpp"
#include "keto/rpc_client/RpcSendQueue.hpp"
#include "keto/rpc_client/RpcClient.hpp"

#include "keto/server_common/StringUtils.hpp"

namespace keto {
namespace rpc_client {

std::string RpcClientProtocol::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcClientProtocol::RpcClientProtocol(int sessionId,const RpcPeer& rpcPeer) : sessionId(sessionId), started(false) {
    rpcReceiveQueuePtr = RpcReceiveQueuePtr(new RpcReceiveQueue(sessionId,rpcPeer));
    rpcSendQueuePtr = RpcSendQueuePtr(new RpcSendQueue(sessionId));
}

RpcClientProtocol::~RpcClientProtocol() {

}

void RpcClientProtocol::start(const RpcSessionSocketPtr& rpcSessionSocket) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->started = true;
    rpcSendQueuePtr->start(rpcSessionSocket);
    rpcReceiveQueuePtr->start(rpcSendQueuePtr);
}

void RpcClientProtocol::preStop() {
    if (this->started) {
        rpcSendQueuePtr->preStop();
        rpcReceiveQueuePtr->preStop();
    }
}

void RpcClientProtocol::stop() {
    if (this->started) {
        rpcSendQueuePtr->stop();
        rpcReceiveQueuePtr->stop();
    }
}

void RpcClientProtocol::abort() {
    if (this->started) {
        rpcSendQueuePtr->abort();
        rpcReceiveQueuePtr->abort();
    }
}

void RpcClientProtocol::join() {
    if (this->started) {
        KETO_LOG_INFO << "[RpcClientProtocol::join] join send queue";
        rpcSendQueuePtr->join();
        KETO_LOG_INFO << "[RpcClientProtocol::join] join receive queu";
        rpcReceiveQueuePtr->join();
        KETO_LOG_INFO << "[RpcClientProtocol::join] after joins";
    }
}

bool RpcClientProtocol::isStarted() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return started;
}

RpcReceiveQueuePtr RpcClientProtocol::getReceiveQueue() {
    return this->rpcReceiveQueuePtr;
}

RpcSendQueuePtr RpcClientProtocol::getSendQueue() {
    return this->rpcSendQueuePtr;
}

}
}