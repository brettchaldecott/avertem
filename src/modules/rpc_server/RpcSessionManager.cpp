//
// Created by Brett Chaldecott on 2021/07/19.
//

#include "keto/rpc_server/RpcSessionManager.hpp"
#include "keto/rpc_server/Exception.hpp"
#include "keto/election_common/Constants.hpp"

#include "keto/server_common/ServerInfo.hpp"

#include "keto/rpc_server/Constants.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

static RpcSessionManagerPtr singleton;

std::string RpcSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSessionManager::RpcSessionManager() : active(true), serverActive(false), sessionSequence(0) {

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
    if (singleton) {
        singleton.reset();
    }
}

RpcSessionManagerPtr RpcSessionManager::getInstance() {
    return singleton;
}

void RpcSessionManager::start() {
    sessionManagerThreadPtr = std::shared_ptr<std::thread>(new std::thread(
            [this]
            {
                this->run();
            }));
}

void RpcSessionManager::stop() {
    KETO_LOG_INFO << "[RpcSessionManager] Deactivate";
    deactivate();
    KETO_LOG_INFO << "[RpcSessionManager] Stop the sessions";
    std::vector<RpcSessionPtr> rpcSessionPtrVectors;
    while((rpcSessionPtrVectors = getSessions()).size()) {
        KETO_LOG_INFO << "[RpcSessionManager] Stop the session : " <<  rpcSessionPtrVectors.size();
        for (RpcSessionPtr rpcSessionPtr : rpcSessionPtrVectors) {
            KETO_LOG_INFO << "[RpcSessionManager] Stop the session : " <<  rpcSessionPtr->getAccount();
            rpcSessionPtr->stop();
            KETO_LOG_INFO << "[RpcSessionManager] Stopped the session : " <<  rpcSessionPtr->getAccount();
        }
        // delay before checking for sessions
        std::this_thread::sleep_for(std::chrono::milliseconds(Constants::DEFAULT_RPC_SERVER_QUEUE_DELAY));
    }
    KETO_LOG_INFO << "[RpcSessionManager] Join and wait for the thread to stop";
    sessionManagerThreadPtr->join();
    KETO_LOG_INFO << "[RpcSessionManager] Session manager is stopped";
}

RpcSessionPtr RpcSessionManager::addSession(boost::asio::ip::tcp::socket&& socket, sslBeast::context& _ctx) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!this->active) {
        // the session is currently in active and new entries cannot be added.
        return RpcSessionPtr();
    }
    RpcSessionPtr rpcSessionPtr = RpcSessionPtr(new RpcSession(++this->sessionSequence, std::move(socket), _ctx));
    rpcSessionPtr->start();
    this->sessionMap[rpcSessionPtr->getSessionId()] = rpcSessionPtr;
    return rpcSessionPtr;
}

RpcSessionPtr RpcSessionManager::getSession(int sessionId) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (this->sessionMap.count(sessionId)) {
        return this->sessionMap[sessionId];
    }
    return RpcSessionPtr();
}

RpcSessionPtr RpcSessionManager::getSession(const std::string& account) {
    for (RpcSessionPtr rpcSessionPtr : getSessions()) {
        if (account == rpcSessionPtr->getAccount() || account == rpcSessionPtr->getAccountHash()) {
            return rpcSessionPtr;
        }
    }
    return RpcSessionPtr();
}

void RpcSessionManager::markAsEndedSession(int sessionId) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!this->sessionMap.count(sessionId)) {
        return;
    }
    this->garbageDeque.push_back(this->sessionMap[sessionId]);
    this->sessionMap.erase(sessionId);
    this->stateCondition.notify_all();
}

std::vector<RpcSessionPtr> RpcSessionManager::getSessions() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    std::vector<RpcSessionPtr> sessions;
    for(std::map<int,RpcSessionPtr>::iterator iter = this->sessionMap.begin();
        iter != this->sessionMap.end(); ++iter) {
        sessions.push_back(iter->second);
    }
    return sessions;
}

std::vector<RpcSessionPtr> RpcSessionManager::getRegisteredSessions() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    std::vector<RpcSessionPtr> sessions;
    for(std::map<int,RpcSessionPtr>::iterator iter = this->sessionMap.begin();
        iter != this->sessionMap.end(); ++iter) {
        if (iter->second->isRegistered()) {
            sessions.push_back(iter->second);
        }
    }
    return sessions;
}

std::vector<RpcSessionPtr> RpcSessionManager::getActiveSessions() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    std::vector<RpcSessionPtr> sessions;
    for(std::map<int,RpcSessionPtr>::iterator iter = this->sessionMap.begin();
        iter != this->sessionMap.end(); ++iter) {
        if (iter->second->isActive()) {
            sessions.push_back(iter->second);
        }
    }
    return sessions;
}

void RpcSessionManager::run() {
    RpcSessionPtr entry;
    //KETO_LOG_INFO << "[RpcSessionManager::run] process sessions";
    while(entry = popGarbageSession()) {
        //KETO_LOG_INFO << "[RpcSessionManager::run] wait for an entry to terminate";
        entry->join();
        //KETO_LOG_INFO << "[RpcSessionManager::run] waiting complete";
    }
    //KETO_LOG_INFO << "[RpcSessionManager::run] session manager thread completed";
}

RpcSessionPtr RpcSessionManager::popGarbageSession() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    while(this->active || !this->sessionMap.empty() || !this->garbageDeque.empty()) {
        if (!this->garbageDeque.empty()) {
            //KETO_LOG_INFO << "[RpcSessionManager::popGarbageSession] pop garbage session [" << this->active
            //    << "][" << this->sessionMap.size() << "][" << this->garbageDeque.size() << "]";
            RpcSessionPtr result = this->garbageDeque.front();
            this->garbageDeque.pop_front();
            return result;
        }
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_SERVER_QUEUE_DELAY));
    }
    return RpcSessionPtr();
}

void RpcSessionManager::deactivate() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->active = false;
    this->stateCondition.notify_all();
}


}
}
