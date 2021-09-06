//
// Created by Brett Chaldecott on 2021/08/11.
//

#include "keto/rpc_client/RpcSessionManager.hpp"

#include <botan/hex.h>

#include "keto/ssl/RootCertificate.hpp"
#include "keto/environment/EnvironmentManager.hpp"

#include "keto/rpc_client/RpcReceiveQueue.hpp"
#include "keto/rpc_client/RpcSendQueue.hpp"
#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/RpcClient.hpp"
#include "keto/rpc_client/RpcClientSession.hpp"


#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/TransactionHelper.hpp"

#include "keto/transaction/Transaction.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "keto/rpc_protocol/PeerResponseHelper.hpp"
#include "keto/rpc_protocol/PeerRequestHelper.hpp"
#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/election_common/PublishedElectionInformationHelper.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"


namespace keto {
namespace rpc_client {

static RpcSessionManagerPtr singleton;

std::string RpcSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


RpcSessionManager::RpcSessionManager() : active(true) {
    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = keto::environment::EnvironmentManager::getInstance()->getConfig();
    threads = Constants::DEFAULT_RPC_CLIENT_THREADS;
    if (config->getVariablesMap().count(Constants::RPC_CLIENT_THREADS)) {
        threads = std::max<int>(Constants::DEFAULT_RPC_CLIENT_THREADS,atoi(config->getVariablesMap()[Constants::RPC_CLIENT_THREADS].as<std::string>().c_str()));
    }

    // setup the ioc threads
    this->ioc = std::make_shared<net::io_context>(threads);

    // setup the context
    this->ctx = std::make_shared<sslBeast::context>(sslBeast::context::tlsv12_client);

    // This holds the root certificate used for verification
    keto::ssl::load_root_certificates(*ctx);
}

RpcSessionManager::~RpcSessionManager() {

}

    // account service management methods
RpcSessionManagerPtr RpcSessionManager::init() {
    if (!singleton) {
        singleton = RpcSessionManagerPtr(new RpcSessionManager());
    }
    return singleton;
}

void RpcSessionManager::fin() {
    singleton.reset();
}

RpcSessionManagerPtr RpcSessionManager::getInstance() {
    return singleton;
}

void RpcSessionManager::start() {
    sessionManagerThreadPtr = std::shared_ptr<std::thread>(new std::thread(
            [this]
            {
                KETO_LOG_INFO << "[RpcSessionManager::start] Start the run method of the session manager";
                this->run();
                KETO_LOG_INFO << "[RpcSessionManager::start] Run method complete";
            }));
}

void RpcSessionManager::postStart() {
    this->threadsVector.reserve(this->threads);
    for(int i = 0; i < this->threads; i++) {
        this->threadsVector.emplace_back(
                [this]
                {
                    this->ioc->run();
                    KETO_LOG_INFO << "[RpcSessionManager::postStart] IOC thread has completed : " << this->ioc->stopped();
                });
    }
}

void RpcSessionManager::stop() {
    deactivate();
    std::vector<RpcClientSessionPtr> rpcClientSessionPtrVectors;
    while((rpcClientSessionPtrVectors = getSessions()).size()) {
        for (RpcClientSessionPtr rpcClientSessionPtr : rpcClientSessionPtrVectors) {
            rpcClientSessionPtr->stop();
        }
    }
    sessionManagerThreadPtr->join();

    KETO_LOG_INFO << "[RpcSessionManager::stop] terminate the threads";
    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
    iter != this->threadsVector.end(); iter++) {
        iter->join();
    }
    KETO_LOG_INFO << "[RpcSessionManager::stop] clear buffers";
    this->threadsVector.clear();
    this->ioc.reset();
    KETO_LOG_ERROR << "[RpcSessionManager::stop] complete";
}


RpcClientSessionPtr RpcSessionManager::addSession(const RpcPeer& rpcPeer) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    KETO_LOG_ERROR << "[RpcSessionManager::addSession] add a session for [" << rpcPeer.getPeer() << "] : " << rpcPeer.getPeered();
    if (!this->active) {
        // the session is currently in active and new entries cannot be added.
        return RpcClientSessionPtr();
    }
    RpcClientSessionPtr rpcClientSessionPtr = RpcClientSessionPtr(new RpcClientSession(
            ++this->sessionSequence, this->ioc,this->ctx,rpcPeer));
    rpcClientSessionPtr->start();
    this->sessionMap[rpcClientSessionPtr->getSessionId()] = rpcClientSessionPtr;
    return rpcClientSessionPtr;

}

RpcClientSessionPtr RpcSessionManager::getFirstSession() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (this->sessionMap.size()) {
        return this->sessionMap.begin()->second;
    }
    return RpcClientSessionPtr();
}

RpcClientSessionPtr RpcSessionManager::getSession(int sessionId) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (this->sessionMap.count(sessionId)) {
        return this->sessionMap[sessionId];
    }
    return RpcClientSessionPtr();
}

RpcClientSessionPtr RpcSessionManager::getSessionByAccount(const std::string& account) {
    for (RpcClientSessionPtr rpcClientSessionPtr : getSessions()) {
        if (account == rpcClientSessionPtr->getAccount() || account == rpcClientSessionPtr->getAccountHash()) {
            return rpcClientSessionPtr;
        }
    }
    return RpcClientSessionPtr();
}

RpcClientSessionPtr RpcSessionManager::getSessionByHost(const std::string& host) {
    for (RpcClientSessionPtr rpcClientSessionPtr : getSessions()) {
        if (host == rpcClientSessionPtr->getHost()) {
            return rpcClientSessionPtr;
        }
    }
    return RpcClientSessionPtr();
}

void RpcSessionManager::markAsEndedSession(int sessionId) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!this->sessionMap.count(sessionId)) {
        return;
    }
    this->garbageDeque.push_back(this->sessionMap[sessionId]);
    this->sessionMap.erase(sessionId);
}

std::vector<RpcClientSessionPtr> RpcSessionManager::getSessions() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    std::vector<RpcClientSessionPtr> sessions;
    for(std::map<int,RpcClientSessionPtr>::iterator iter = this->sessionMap.begin();
        iter != this->sessionMap.end(); ++iter) {
        sessions.push_back(iter->second);
    }
    return sessions;
}

std::vector<RpcClientSessionPtr> RpcSessionManager::getRegisteredSessions() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    std::vector<RpcClientSessionPtr> sessions;
    for(std::map<int,RpcClientSessionPtr>::iterator iter = this->sessionMap.begin();
    iter != this->sessionMap.end(); ++iter) {
        if (iter->second->isRegistered()) {
            sessions.push_back(iter->second);
        }
    }
    return sessions;
}

std::vector<RpcClientSessionPtr> RpcSessionManager::getActiveSessions() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    std::vector<RpcClientSessionPtr> sessions;
    for(std::map<int,RpcClientSessionPtr>::iterator iter = this->sessionMap.begin();
    iter != this->sessionMap.end(); ++iter) {
        if (iter->second->isActive()) {
            sessions.push_back(iter->second);
        }
    }
    return sessions;
}

bool RpcSessionManager::isActive() {
    return this->active;
}

void RpcSessionManager::run() {
    RpcClientSessionPtr entry;
    KETO_LOG_INFO << "[RpcSessionManager::run] Process entry";
    while(entry = popGarbageSession()) {
        KETO_LOG_INFO << "[RpcSessionManager::run] join an entry";
        entry->join();
        KETO_LOG_INFO << "[RpcSessionManager::run] finished the join";
    }
    KETO_LOG_INFO << "[RpcSessionManager::run] run complete";
}

RpcClientSessionPtr RpcSessionManager::popGarbageSession() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    while(this->active || !this->sessionMap.empty() || !this->garbageDeque.empty()) {
        if (!this->garbageDeque.empty()) {
            RpcClientSessionPtr result = this->garbageDeque.front();
            this->garbageDeque.pop_front();
            return result;
        }
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_CLIENT_QUEUE_DELAY));
    }
    return RpcClientSessionPtr();
}

void RpcSessionManager::deactivate() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->active = false;
    this->stateCondition.notify_all();
}

}
}
