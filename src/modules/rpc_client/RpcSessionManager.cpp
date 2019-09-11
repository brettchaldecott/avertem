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

namespace keto {
namespace rpc_client {

namespace ketoEnv = keto::environment;

static RpcSessionManagerPtr singleton;

std::string RpcSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSessionManager::RpcSessionManager() : peered(true) {
    
    this->ioc = std::make_shared<boost::asio::io_context>();
    
    this->ctx = std::make_shared<boostSsl::context>(boostSsl::context::sslv23_client);

    // This holds the root certificate used for verification
    keto::ssl::load_root_certificates(*ctx);
    
    // retrieve the configuration
    std::shared_ptr<ketoEnv::Config> config = ketoEnv::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::PEERS)) {
        KETO_LOG_DEBUG << "Load the peers";
        this->configuredPeersString = config->getVariablesMap()[Constants::PEERS].
                as<std::string>();
    }

    threads = Constants::DEFAULT_RPC_CLIENT_THREADS;
    if (config->getVariablesMap().count(Constants::RPC_CLIENT_THREADS)) {
        threads = std::max<int>(1,atoi(config->getVariablesMap()[Constants::RPC_CLIENT_THREADS].as<std::string>().c_str()));
    }

    PeerStore::init();
}

RpcSessionManager::~RpcSessionManager() {
    PeerStore::fin();
}

void RpcSessionManager::setPeers(const std::vector<std::string>& peers, bool peered) {
    PeerStore::getInstance()->setPeers(peers);
    this->peered = peered;
    for (std::vector<std::string>::const_iterator iter = peers.begin();
            iter != peers.end(); iter++) {
        KETO_LOG_DEBUG << "Add the new entry : " << (*iter);
        RpcPeer rpcPeer((*iter),this->peered);
        this->sessionMap[(*iter)] = std::make_shared<RpcSession>(
                this->ioc,
                this->ctx,rpcPeer);
        this->sessionMap[(*iter)]->run();
    }
}

void RpcSessionManager::reconnect(const RpcPeer& rpcPeer) {
    KETO_LOG_DEBUG << "The reconnect count : " << (std::string)rpcPeer;
    this->sessionMap.erase((std::string)rpcPeer);
    if (rpcPeer.getReconnectCount() >= Constants::SESSION::MAX_RETRY_COUNT) {
        // force a reconnect to the peers
        setPeers(keto::server_common::StringUtils(
                this->configuredPeersString).tokenize(","),false);
        return;
    }
    KETO_LOG_DEBUG << "Setup the session";
    this->sessionMap[(std::string)rpcPeer] = std::make_shared<RpcSession>(
                this->ioc,
                this->ctx,rpcPeer);
    std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));
    KETO_LOG_INFO << "Attempt to reconnect to : " << rpcPeer.getPeer();
    this->sessionMap[(std::string)rpcPeer]->run();
    KETO_LOG_INFO << "After the reconnect";
}

std::vector<std::string> RpcSessionManager::listPeers() {
    std::vector<std::string> keys;
    std::transform(
        this->sessionMap.begin(),
        this->sessionMap.end(),
        std::back_inserter(keys),
        [](const std::map<std::string,std::shared_ptr<keto::rpc_client::RpcSession>>::value_type 
            &pair){return pair.first;});
    return keys;
}

std::vector<std::string> RpcSessionManager::listAccountPeers() {
    std::vector<std::string> keys;
    std::transform(
            this->accountSessionMap.begin(),
            this->accountSessionMap.end(),
            std::back_inserter(keys),
            [](const std::map<std::string,std::shared_ptr<keto::rpc_client::RpcSession>>::value_type
               &pair){return pair.first;});
    return keys;
}

void RpcSessionManager::setAccountSessionMapping(const std::string& account,
            const RpcSessionPtr& rpcSessionPtr) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    this->accountSessionMap[account] = rpcSessionPtr;
}

void RpcSessionManager::removeAccountSessionMapping(const std::string& account) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (!this->accountSessionMap.count(account)) {
        return;
    }
    RpcSessionPtr rpcSessionPtr = this->accountSessionMap[account];
    this->accountSessionMap.erase(account);
    this->sessionMap.erase(rpcSessionPtr->getPeer().getPeer());
}

bool RpcSessionManager::hasAccountSessionMapping(const std::string& account) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->accountSessionMap.count(account)) {
        return true;
    }
    return false;
}

RpcSessionPtr RpcSessionManager::getAccountSessionMapping(const std::string& account) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    return this->accountSessionMap[account];
}

RpcSessionPtr RpcSessionManager::getDefaultPeer() {
    if (this->sessionMap.size()) {
        return this->sessionMap.begin()->second;
    }
    return RpcSessionPtr();
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
        KETO_LOG_DEBUG << "The peer is : " << (*iter);
        RpcPeer rpcPeer((*iter),this->peered);
        this->sessionMap[(*iter)] = std::make_shared<RpcSession>(
                this->ioc,
                this->ctx,rpcPeer);
        this->sessionMap[(*iter)]->run();
    }

    this->threadsVector.reserve(this->threads);
    for(int i = 0; i < this->threads; i++) {
        this->threadsVector.emplace_back(
        [this]
        {
            this->ioc->run();
        });
    }
    KETO_LOG_INFO << "[RpcSessionManager::postStart] All the threads have been started : " << this->threads;
    
}

void RpcSessionManager::stop() {
    if (this->ioc) {
        this->ioc->stop();
    }

    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
            iter != this->threadsVector.end(); iter++) {
        iter->join();
    }

    this->threadsVector.clear();
    
    this->sessionMap.clear();
}

keto::event::Event RpcSessionManager::activatePeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    std::vector<std::string> peers = this->listAccountPeers();
    KETO_LOG_DEBUG << "[RpcSessionManager::activatePeer] activating the state of this node with its peer : " << peers.size();
    this->activated = rpcPeerHelper.isActive();
    for (std::string peer : peers)
    {
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
        if (rpcSessionPtr) {
            try {
                KETO_LOG_DEBUG << "[RpcSessionManager::activatePeer] activate the peer : " << rpcSessionPtr->getPeer().getPeer();
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

keto::event::Event RpcSessionManager::requestBlockSync(const keto::event::Event& event) {
    keto::proto::SignedBlockBatchRequest request = keto::server_common::fromEvent<keto::proto::SignedBlockBatchRequest>(event);
    RpcSessionPtr rcpSessionPtr = getDefaultPeer();
    if (rcpSessionPtr) {
        try {
            rcpSessionPtr->requestBlockSync(request);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : " << ex.what();
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : unknown cause";
        }
    } else {
        // this will force a call to the RPC server to sync
        KETO_LOG_INFO << "[RpcSessionManager::requestBlockSync] No upstream connections forcing the request down stream";
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                keto::server_common::Events::RPC_SERVER_REQUEST_BLOCK_SYNC,request));
    }

    return event;
}

keto::event::Event RpcSessionManager::pushBlock(const keto::event::Event& event) {
    std::vector<std::string> peers = this->listAccountPeers();
    for (std::string peer : peers) {
        RpcSessionPtr rpcSessionPtr = getAccountSessionMapping(peer);
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
    
    KETO_LOG_DEBUG << "Check if routing to a parent is possible : " <<
            messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX)<< std::endl;
    // check if there is a peer matching the target account hash this would be pure luck
    if (this->hasAccountSessionMapping(
            messageWrapper.account_hash())) {
        KETO_LOG_DEBUG << "The parent was found " << 
                messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX)
               ;
        this->getAccountSessionMapping(
                messageWrapper.account_hash())->routeTransaction(messageWrapper);
        
    } else {
        // route to the default account which is the first peer in the list
        KETO_LOG_DEBUG << "Get the default peer and route";
        RpcSessionPtr rpcSessionPtr = getDefaultPeer();
        if (rpcSessionPtr) {
            KETO_LOG_DEBUG << "Route to the default peer";
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
            KETO_LOG_DEBUG << "No default peer exists throw an exception";
            
            std::stringstream ss;
            ss << "No default route for [" << 
                messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
            BOOST_THROW_EXCEPTION(keto::rpc_client::NoDefaultRouteAvailableException(
                    ss.str()));
        }
        
    }
    
    KETO_LOG_DEBUG << "Process successfully";
    
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    std::stringstream ss;
    ss << "Routed to the server peer [" << 
            messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
    response.set_result(ss.str());
    KETO_LOG_DEBUG << "Return the result of the routing";
    
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}


keto::event::Event RpcSessionManager::electBlockProducer(const keto::event::Event& event) {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());

    keto::election_common::ElectionMessageProtoHelper electionMessageProtoHelper(
        keto::server_common::fromEvent<keto::proto::ElectionMessage>(event));

    std::vector<std::string> peers = this->listAccountPeers();
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
