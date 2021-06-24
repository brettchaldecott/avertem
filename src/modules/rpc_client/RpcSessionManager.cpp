/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcSessionManager.cpp
 * Author: ubuntu
 * 
 * Created on January 22, 2018, 1:18 PM
 */

#include <map>
#include <chrono>
#include <thread>

#include "keto/ssl/RootCertificate.hpp"

#include "keto/rpc_client/RpcSessionManager.hpp"
#include "keto/rpc_client/RpcSession.hpp"

#include "keto/environment/EnvironmentManager.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/transaction/Transaction.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/Constants.hpp"

#include "keto/rpc_client/PeerStore.hpp"
#include "keto/rpc_client/Exception.hpp"
#include "keto/rpc_client/PeerStore.hpp"

#include "keto/election_common/ElectionMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_client {

namespace ketoEnv = keto::environment;

static RpcSessionManagerPtr singleton;

std::string RpcSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSessionManager::RpcSessionManager() : sessionSequence(0), peered(true), activated(false), terminated(false), networkState(false), sessionCount(0) {

    RpcSession::RpcSessionLifeCycleManager::init();
    // retrieve the configuration
    std::shared_ptr<ketoEnv::Config> config = ketoEnv::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::PEERS)) {
        this->configuredPeersString = config->getVariablesMap()[Constants::PEERS].
                as<std::string>();
    }

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


    PeerStore::init();
}

RpcSessionManager::~RpcSessionManager() {
    PeerStore::fin();
    RpcSession::RpcSessionLifeCycleManager::fin();
}

void RpcSessionManager::setPeers(const std::vector<std::string>& peers, bool peered) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);

    // check if terminated
    if (this->terminated) {
        KETO_LOG_INFO << "The session manager is terminated ignoring the peer";
        return;
    }

    // remove the account and session mappings
    this->peered = peered;
    PeerStore::getInstance()->setPeers(peers);
    this->accountSessionMap.clear();
    this->sessionMap.clear();

    // delay the connection by 10 seconds
    for (std::vector<std::string>::const_iterator iter = peers.begin();
            iter != peers.end(); iter++) {
        try {
            RpcPeer rpcPeer((*iter), this->peered);
            KETO_LOG_INFO << "Connect to peered server : " << rpcPeer.getPeer();
            RpcSessionPtr rpcSessionPtr = std::make_shared<RpcSession>(
                    getNextSessionId(),
                    this->ioc,
                    this->ctx, rpcPeer);
            rpcSessionPtr->run();
            this->sessionMap[(*iter)] = rpcSessionPtr;
            KETO_LOG_INFO << "Started the new session : " << rpcPeer.getPeer();
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::setPeers] Failed to set a peer : " << boost::diagnostic_information_what(ex, true);
            KETO_LOG_ERROR << "[RpcSessionManager::setPeers] cause : " << ex.what();
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::setPeers] Failed to set to a peer : " << boost::diagnostic_information_what(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::setPeers] Failed to set to a peer : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcSessionManager::setPeers] Failed to set to a peer : unknown";
        }
    }
    KETO_LOG_INFO << "Reconection to peers has been completed";
}

void RpcSessionManager::reconnect(RpcPeer& rpcPeer) {
    {
        std::lock_guard<std::recursive_mutex> guard(this->classMutex);

        // remove the account and session mappings
        RpcSessionPtr rpcSessionPtr = this->sessionMap[(std::string)rpcPeer];
        if (rpcSessionPtr && !rpcSessionPtr->getAccountHash().empty()) {
            this->accountSessionMap.erase(rpcSessionPtr->getAccountHash());
        }
        this->sessionMap.erase((std::string)rpcPeer);

        // check if terminated
        if (this->terminated) {
            KETO_LOG_INFO << "The session manager is terminated ignoring the peer " << rpcPeer.getPeer();
            return;
        }
        rpcPeer.incrementReconnectCount();
        if (rpcPeer.getReconnectCount() >= Constants::SESSION::MAX_RETRY_COUNT) {
            KETO_LOG_INFO << "Retry limit reached diconnecting from host " << rpcPeer.getPeer();
            // force a reconnect to the peers
            //setPeers(keto::server_common::StringUtils(
            //        this->configuredPeersString).tokenize(","),false);
            return;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));
    {
        std::lock_guard<std::recursive_mutex> guard(this->classMutex);
        try {
            KETO_LOG_INFO << "Attempt to reconnect to : " << rpcPeer.getPeer();
            RpcSessionPtr rpcSessionPtr = std::make_shared<RpcSession>(
                    getNextSessionId(),
                    this->ioc,
                    this->ctx, rpcPeer);
            rpcSessionPtr->run();
            this->sessionMap[(std::string) rpcPeer] = rpcSessionPtr;
            KETO_LOG_INFO << "After the reconnect : " << rpcPeer.getPeer();
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::reconnect] Failed to reconnect to a peer : "
                           << boost::diagnostic_information_what(ex, true);
            KETO_LOG_ERROR << "[RpcSessionManager::reconnect] cause : " << ex.what();
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::reconnect] Failed to reconnect to a peer : "
                           << boost::diagnostic_information_what(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::reconnect] Failed to reconnect to a peer : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcSessionManager::reconnect] Failed to reconnect to a peer : unknown";
        }
    }
}


std::vector<keto::rpc_client::RpcSessionPtr> RpcSessionManager::getPeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<keto::rpc_client::RpcSessionPtr> peers;
    for (std::map<std::string,keto::rpc_client::RpcSessionPtr>::iterator iter =
            this->sessionMap.begin(); iter != this->sessionMap.end(); iter++) {
        peers.push_back(iter->second);
    }
    KETO_LOG_INFO << "[RpcSessionManager::getPeers] Get the list of peers : " << peers.size();
    return peers;
}

void RpcSessionManager::clearSessions() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    this->sessionMap.clear();
    this->accountSessionMap.clear();
}

int RpcSessionManager::getNextSessionId() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    return ++sessionSequence;
}

std::vector<std::string> RpcSessionManager::listPeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<std::string> keys;
    std::transform(
        this->sessionMap.begin(),
        this->sessionMap.end(),
        std::back_inserter(keys),
        [](const std::map<std::string,keto::rpc_client::RpcSessionPtr>::value_type
            &pair){return pair.first;});
    return keys;
}

std::vector<std::string> RpcSessionManager::listAccountPeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<std::string> keys;
    std::transform(
            this->accountSessionMap.begin(),
            this->accountSessionMap.end(),
            std::back_inserter(keys),
            [](const std::map<std::string,keto::rpc_client::RpcSessionPtr>::value_type
               &pair){return pair.first;});
    return keys;
}

std::vector<std::string> RpcSessionManager::listActiveAccountPeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<std::string> keys;
    for (std::map<std::string,keto::rpc_client::RpcSessionPtr>::iterator iter =
            this->accountSessionMap.begin(); iter != this->accountSessionMap.end(); iter++) {
        if (iter->second && iter->second->isActive()) {
            keys.push_back(iter->first);
        }
    }
    return keys;
}

void RpcSessionManager::setAccountSessionMapping(const std::string& peer, const std::string& account) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    this->accountSessionMap[account] = this->sessionMap[peer];
}

void RpcSessionManager::removeSession(const RpcPeer& rpcPeer, const std::string& account) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    if (this->sessionMap.count((std::string)rpcPeer)) {
        this->sessionMap.erase((std::string) rpcPeer);
    }
    if (this->accountSessionMap.count(account)) {
        this->accountSessionMap.erase(account);
    }

}

bool RpcSessionManager::isTerminated() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    return this->terminated;
}

void RpcSessionManager::terminate() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    this->terminated = true;
}


//int RpcSessionManager::incrementSessionCount() {
//    std::unique_lock<std::mutex> unique_lock(this->sessionClassMutex);
//    int result = ++this->sessionCount;
//    this->stateCondition.notify_all();
//    return result;
//}
//
//int RpcSessionManager::decrementSessionCount() {
//    std::unique_lock<std::mutex> unique_lock(this->sessionClassMutex);
//    int result = --this->sessionCount;
//    this->stateCondition.notify_all();
//    return result;
//}

void RpcSessionManager::waitForSessionEnd() {
    KETO_LOG_ERROR << "[RpcSessionManager::waitForSessionEnd] waitForSessionEnd : " << this->sessionCount;

    bool waitForTimeout = false;
    int sessions = 0;
    std::vector<keto::rpc_client::RpcSessionPtr> peers;
    RpcSession::RpcSessionLifeCycleManager::getInstance()->terminate();
    while(sessions = getSessionCount(waitForTimeout) && (peers = getActivePeers()).size() && !RpcSession::RpcSessionLifeCycleManager::getInstance()->isTerminated()){
        KETO_LOG_ERROR << "[RpcSessionManager::waitForSessionEnd] stop the peers";
        for (RpcSessionPtr rpcSessionPtr : peers) {
            if (rpcSessionPtr) {
                try {
                    rpcSessionPtr->closeSession();
                } catch (keto::common::Exception &ex) {
                    KETO_LOG_ERROR << "[RpcSessionManager::preStop] Failed to close the session : " << ex.what();
                    KETO_LOG_ERROR << "[RpcSessionManager::preStop] Cause : "
                                   << boost::diagnostic_information(ex, true);
                } catch (boost::exception &ex) {
                    KETO_LOG_ERROR << "[RpcSessionManager::preStop] Failed to close the session : "
                                   << boost::diagnostic_information(ex, true);
                } catch (std::exception &ex) {
                    KETO_LOG_ERROR << "[RpcSessionManager::preStop] Failed to close the session : " << ex.what();
                } catch (...) {
                    KETO_LOG_ERROR << "[RpcSessionManager::preStop] Failed to close the session : unknown cause";
                }
            }
        }
        waitForTimeout = true;
        KETO_LOG_ERROR << "[RpcSessionManager::waitForSessionEnd] waitForSessionEnd [" << sessions << "][" << peers.size() << "]";
    }
    // clear the sessions
    clearSessions();
}

int RpcSessionManager::getSessionCount(bool waitForTimeout) {
    {
        std::unique_lock<std::mutex> unique_lock(this->sessionClassMutex);
        if (waitForTimeout) {
            this->stateCondition.wait_for(unique_lock, std::chrono::milliseconds(1000));
        }
    }
    return getPeers().size();
}

bool RpcSessionManager::hasNetworkState() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    return this->networkState;
}

void RpcSessionManager::activateNetworkState() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    this->networkState = true;
    keto::proto::RequestNetworkState requestNetworkState;
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RequestNetworkState>(
            keto::server_common::Events::ACTIVATE_NETWORK_STATE_SERVER,requestNetworkState));
}

bool RpcSessionManager::hasAccountSessionMapping(const std::string& account) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    if (this->accountSessionMap.count(account)) {
        return true;
    }
    return false;
}

RpcSessionPtr RpcSessionManager::getAccountSessionMapping(const std::string& account) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    return this->accountSessionMap[account];
}

RpcSessionPtr RpcSessionManager::getDefaultPeer() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    if (this->sessionMap.size()) {
        return this->sessionMap.begin()->second;
    }
    return RpcSessionPtr();
}

RpcSessionPtr RpcSessionManager::getActivePeer() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    for (std::map<std::string,keto::rpc_client::RpcSessionPtr>::iterator iter =
            this->accountSessionMap.begin(); iter != this->accountSessionMap.end(); iter++) {
        if (iter->second && iter->second->isActive()) {
            return iter->second;
        }
    }
    return RpcSessionPtr();
}

std::vector<RpcSessionPtr> RpcSessionManager::getActivePeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<RpcSessionPtr> result;
    for (std::map<std::string,keto::rpc_client::RpcSessionPtr>::iterator iter =
            this->accountSessionMap.begin(); iter != this->accountSessionMap.end(); iter++) {
        if (iter->second && iter->second->isActive()) {
            result.push_back(iter->second);
        }
    }
    return result;
}

std::vector<RpcSessionPtr> RpcSessionManager::getRegisteredPeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<RpcSessionPtr> result;
    for (std::map<std::string,keto::rpc_client::RpcSessionPtr>::iterator iter =
            this->accountSessionMap.begin(); iter != this->accountSessionMap.end(); iter++) {
        if (iter->second && iter->second->isRegistered()) {
            result.push_back(iter->second);
        }
    }
    return result;
}

std::vector<RpcSessionPtr> RpcSessionManager::getAccountPeers() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    std::vector<RpcSessionPtr> result;
    for (std::map<std::string,keto::rpc_client::RpcSessionPtr>::iterator iter =
            this->accountSessionMap.begin(); iter != this->accountSessionMap.end(); iter++) {
        result.push_back(iter->second);
    }
    return result;
}

bool RpcSessionManager::registeredAccounts() {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    return !this->accountSessionMap.empty();
}

RpcSessionManagerPtr RpcSessionManager::init() {
    return singleton = std::shared_ptr<RpcSessionManager>(new RpcSessionManager());
}

void RpcSessionManager::fin() {
    singleton.reset();
}

RpcSessionManagerPtr RpcSessionManager::getInstance() {
    return singleton;
}



void RpcSessionManager::start() {
    KETO_LOG_INFO << "The start has been called";
    // Run the I/O service on the requested number of threads
    
}

void RpcSessionManager::postStart() {
    KETO_LOG_INFO << "The post start has been called";
    std::vector<std::string> peers = PeerStore::getInstance()->getPeers();
    if (!peers.size()) {
        peers = keto::server_common::StringUtils(
                        this->configuredPeersString).tokenize(",");
        this->peered = false;
    }

    KETO_LOG_INFO << "After retrieving the peers.";
    for (std::vector<std::string>::iterator iter = peers.begin();
         iter != peers.end(); iter++) {
        try {
            KETO_LOG_INFO << "Connect to peer : " << (*iter);
            RpcPeer rpcPeer((*iter), this->peered);
            RpcSessionPtr rpcSessionPtr = std::make_shared<RpcSession>(
                    getNextSessionId(),
                    this->ioc,
                    this->ctx, rpcPeer);
            rpcSessionPtr->run();
            this->sessionMap[(std::string) rpcPeer] = rpcSessionPtr;
            KETO_LOG_INFO << "Added peer connection : " << (*iter);
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::postStart] Failed to start a peer : " << boost::diagnostic_information_what(ex, true);
            KETO_LOG_ERROR << "[RpcSessionManager::postStart] cause : " << ex.what();
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::postStart] Failed to start a peer : " << boost::diagnostic_information_what(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::postStart] Failed to start a peer : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcSessionManager::postStart] Failed to start a peer : unknown";
        }
    }

    this->threadsVector.reserve(this->threads);
    for(int i = 0; i < this->threads; i++) {
        this->threadsVector.emplace_back(
        [this]
        {
            this->ioc->run();
            KETO_LOG_INFO << "[RpcSessionManager::postStart] IOC thread has completed : " << this->ioc->stopped();
        });
    }
    KETO_LOG_INFO << "[RpcSessionManager::postStart] All the threads have been started : " << this->threads;
    
}

void RpcSessionManager::preStop() {
    // terminated
    KETO_LOG_ERROR << "[RpcSessionManager::preStop] Terminate the session manager";
    terminate();

    KETO_LOG_ERROR << "[RpcSessionManager::preStop] wait for the sessions";
    this->waitForSessionEnd();

    // stop the threads
    KETO_LOG_ERROR << "[RpcSessionManager::preStop] terminate the threads";
    if (this->ioc) {
        this->ioc->stop();
    }

    KETO_LOG_ERROR << "[RpcSessionManager::preStop] terminate the threads";
    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
         iter != this->threadsVector.end(); iter++) {
        iter->join();
    }
    this->threadsVector.clear();
    this->ioc.reset();
}

void RpcSessionManager::stop() {

}

keto::event::Event RpcSessionManager::activatePeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    std::vector<std::string> peers = this->listAccountPeers();
    this->activated = rpcPeerHelper.isActive();
    for (std::string peer : peers)
    {
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->activatePeer(rpcPeerHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : unknown cause";
            }
        }
    }
    return event;
}

keto::event::Event RpcSessionManager::requestNetworkState(const keto::event::Event& event) {
    KETO_LOG_INFO << "[RpcSessionManager::requestNetworkState] The client request network state has been set.";
    this->networkState = false;
    return event;
}

keto::event::Event RpcSessionManager::activateNetworkState(const keto::event::Event& event) {
    KETO_LOG_INFO << "[RpcSessionManager::activateNetworkState] The network status is active.";
    this->networkState = true;
    return event;
}

keto::event::Event RpcSessionManager::requestBlockSync(const keto::event::Event& event) {
    keto::proto::SignedBlockBatchRequest request = keto::server_common::fromEvent<keto::proto::SignedBlockBatchRequest>(event);
    std::vector<RpcSessionPtr> rcpSessionPtrs = this->getActivePeers();
    //int usePeers = std::rand() % 2;
    //if (!rcpSessionPtrs.size() && usePeers) {
    //    rcpSessionPtrs = this->getAccountPeers();
    //}
    KETO_LOG_INFO << "[RpcSessionManager::requestBlockSync] Making request to the following peers [" << rcpSessionPtrs.size() << "]";


    if (rcpSessionPtrs.size()) {
        // select a random rpc service if there are more than one upstream providers
        RpcSessionPtr currentSessionPtr = rcpSessionPtrs[0];
        if (rcpSessionPtrs.size()>1) {
            for (RpcSessionPtr rpcSessionPtr : rcpSessionPtrs) {
                if (rpcSessionPtr && (rpcSessionPtr->getLastBlockTouch() > currentSessionPtr->getLastBlockTouch())) {
                    currentSessionPtr = rpcSessionPtr;
                }
            }
        }
        try {
            currentSessionPtr->requestBlockSync(request);
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : "
                           << ex.what();
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Cause : "
                           << boost::diagnostic_information(ex, true);
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : "
                           << boost::diagnostic_information(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : "
                           << ex.what();
        } catch (...) {
            KETO_LOG_ERROR
                << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : unknown cause";
        }
    } else {
        // attempt to call children for synchronization.
        KETO_LOG_INFO << "[RpcSessionManager::requestBlockSync] No upstream connections forcing the request down stream";
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                keto::server_common::Events::RPC_SERVER_REQUEST_BLOCK_SYNC,request));
    }

    return event;
}

keto::event::Event RpcSessionManager::pushBlock(const keto::event::Event& event) {
    KETO_LOG_INFO << "[RpcSessionManager::pushBlock] push block to client";
    for (RpcSessionPtr rpcSessionPtr : getRegisteredPeers()) {
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->pushBlock(keto::server_common::fromEvent<keto::proto::SignedBlockWrapperMessage>(event));
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushBlock] Failed to push block : " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::pushBlock] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushBlock] Failed to push block : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushBlock] Failed to push block : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushBlock] Failed to push block : unknown cause";
            }
        }
    }

    return event;
}

keto::event::Event RpcSessionManager::routeTransaction(const keto::event::Event& event) {
    
    keto::proto::MessageWrapper messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(messageWrapper);

    // check if there is a peer matching the target account hash this would be pure luck
    if (this->hasAccountSessionMapping(
            messageWrapper.account_hash())) {
        this->getAccountSessionMapping(
                messageWrapper.account_hash())->routeTransaction(messageWrapper);
        
    } else {
        // route to the default account which is the first peer in the list
        RpcSessionPtr rpcSessionPtr = getDefaultPeer();
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->routeTransaction(messageWrapper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::routeTransaction] Failed route transaction : " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::routeTransaction] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::routeTransaction] Failed route transaction : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::routeTransaction] Failed route transaction : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::routeTransaction] Failed route transaction : unknown cause";
            }
        } else {
            std::stringstream ss;
            ss << "No default route for [" << 
                messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
            BOOST_THROW_EXCEPTION(keto::rpc_client::NoDefaultRouteAvailableException(
                    ss.str()));
        }
        
    }
    
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    std::stringstream ss;
    ss << "Routed to the server peer [" << 
            messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
    response.set_result(ss.str());
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}


keto::event::Event RpcSessionManager::electBlockProducer(const keto::event::Event& event) {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());

    keto::election_common::ElectionMessageProtoHelper electionMessageProtoHelper(
        keto::server_common::fromEvent<keto::proto::ElectionMessage>(event));

    std::vector<std::string> peers = this->listActiveAccountPeers();
    for (int index = 0; (index < keto::server_common::Constants::ELECTION::ELECTOR_COUNT) && (peers.size()); index++) {

        // distribution
        std::string peer;
        if (peers.size() > 1) {
            std::uniform_int_distribution<int> distribution(0, peers.size() - 1);
            distribution(stdGenerator);
            int pos = distribution(stdGenerator);
            peer = peers[pos];
            peers.erase(peers.begin() + pos);
        } else {
            peer = peers[0];
            peers.clear();
        }

        // get the account
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->electBlockProducer();
                electionMessageProtoHelper.addAccount(keto::asn1::HashHelper(peer));
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducer] Failed to push block : " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducer] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducer] Failed to push block : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducer] Failed to push block : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducer] Failed to push block : unknown cause";
            }
        }
    }

    return keto::server_common::toEvent<keto::proto::ElectionMessage>(electionMessageProtoHelper);
}

keto::event::Event RpcSessionManager::electBlockProducerPublish(const keto::event::Event& event) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));

    std::vector<std::string> peers = this->listAccountPeers();
    for (std::string peer : peers) {

        // get the account
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->electBlockProducerPublish(electionPublishTangleAccountProtoHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle change: " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle changes : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle changes : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle changes : unknown cause";
            }
        }
    }

    return event;
}


keto::event::Event RpcSessionManager::electBlockProducerConfirmation(const keto::event::Event& event) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));
    std::vector<std::string> peers = this->listAccountPeers();
    for (std::string peer : peers) {

        // get the account
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->electBlockProducerConfirmation(electionConfirmationHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle change: " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle changes : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle changes : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle changes : unknown cause";
            }
        }
    }

    return event;
}


keto::event::Event RpcSessionManager::pushRpcPeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    std::vector<std::string> peers = this->listAccountPeers();
    for (std::string peer : peers) {

        // get the account
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
        if (rpcSessionPtr) {
            try {
                rpcSessionPtr->pushRpcPeer(rpcPeerHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushToRpcPeer] Failed to push peer to rpc peers: " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::pushToRpcPeer] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushToRpcPeer] Failed to push peer to rpc peers : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushToRpcPeer] Failed to push peer to rpc peers : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::pushToRpcPeer] Failed to push peer to rpc peers : unknown cause";
            }
        }
    }

    return event;
}

bool RpcSessionManager::isActivated() {
    return this->activated;
}

}
}
