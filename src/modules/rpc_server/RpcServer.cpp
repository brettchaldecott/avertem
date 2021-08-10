/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcServer.cpp
 * Author: ubuntu
 * 
 * Created on January 22, 2018, 7:08 AM
 */

#include <string>
#include <sstream>
#include <map>
#include <queue>
#include <vector>

#include "SoftwareConsensus.pb.h"
#include "HandShake.pb.h"
#include "Route.pb.h"
#include "BlockChain.pb.h"
#include "Protocol.pb.h"
#include "KeyStore.pb.h"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <botan/hex.h>
#include <botan/base64.h>
#include <google/protobuf/message_lite.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/host_name.hpp>

#include "keto/common/Log.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/rpc_server/RpcServer.hpp"
#include "keto/rpc_server/Constants.hpp"
#include "keto/rpc_server/RpcServerSession.hpp"
#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"
#include "keto/rpc_protocol/PeerRequestHelper.hpp"
#include "keto/rpc_protocol/PeerResponseHelper.hpp"
#include "keto/ssl/ServerCertificate.hpp"
#include "keto/environment/EnvironmentManager.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/Events.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/transaction/Transaction.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/server_common/TransactionHelper.hpp"

#include "keto/software_consensus/ModuleConsensusValidationMessageHelper.hpp"
#include "keto/rpc_server/RpcServerSession.hpp"
#include "keto/rpc_server/Exception.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"

#include "keto/election_common/ElectionMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"
#include "keto/election_common/ElectionResultCache.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"
#include "keto/election_common/PublishedElectionInformationHelper.hpp"


#include "keto/rpc_protocol/NetworkStatusHelper.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

static RpcServerPtr singleton;

namespace ketoEnv = keto::environment;

std::string RpcServer::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcServer::RpcServer() : externalHostname(""), serverActive(false), networkState(true), terminated(false) {
    // retrieve the configuration
    std::shared_ptr<ketoEnv::Config> config = ketoEnv::EnvironmentManager::getInstance()->getConfig();
    
    serverIp = boost::asio::ip::make_address(Constants::DEFAULT_IP);
    if (config->getVariablesMap().count(Constants::IP_ADDRESS)) {
        serverIp = boost::asio::ip::make_address(
                config->getVariablesMap()[Constants::IP_ADDRESS].as<std::string>());
    }
    
    
    
    serverPort = Constants::DEFAULT_PORT_NUMBER;
    if (config->getVariablesMap().count(Constants::PORT_NUMBER)) {
        serverPort = static_cast<unsigned short>(
                atoi(config->getVariablesMap()[Constants::PORT_NUMBER].as<std::string>().c_str()));
    }
    
    threads = Constants::DEFAULT_HTTP_THREADS;
    if (config->getVariablesMap().count(Constants::HTTP_THREADS)) {
        threads = std::max<int>(1,atoi(config->getVariablesMap()[Constants::HTTP_THREADS].as<std::string>().c_str()));
    }


    // setup the external hostname information
    if (config->getVariablesMap().count(Constants::EXTERNAL_HOSTNAME)) {
        this->externalHostname =
                config->getVariablesMap()[Constants::EXTERNAL_HOSTNAME].as<std::string>();
    } else {
        this->externalHostname = boost::asio::ip::host_name();
    }

    RpcSessionManager::init();
}

RpcServer::~RpcServer() {
    RpcSessionManager::fin();
}

RpcServerPtr RpcServer::init() {
    return singleton = std::shared_ptr<RpcServer>(new RpcServer());
}

void RpcServer::fin() {
    singleton.reset();
}

RpcServerPtr RpcServer::getInstance() {
    return singleton;
}

void RpcServer::start() {
    
    
    // setup the beginnings of the peer cache of the external ip address has been supplied
    std::shared_ptr<ketoEnv::Config> config = ketoEnv::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::EXTERNAL_IP_ADDRESS)) {
        this->setExternalIp(boost::asio::ip::make_address(
                config->getVariablesMap()[Constants::EXTERNAL_IP_ADDRESS].as<std::string>()));
    }
    
    // The io_context is required for all I/O
    this->ioc = std::make_shared<net::io_context>(this->threads);

    // The SSL context is required, and holds certificates
    this->contextPtr = std::make_shared<sslBeast::context>(sslBeast::context::tlsv12);
    
    // This holds the self-signed certificate used by the server
    load_server_certificate(*(this->contextPtr));

    // init the rpc session manager
    RpcSessionManager::getInstance()->start();

    // Create and launch a listening port
    rpcListenerPtr = RpcListenerPtr(new RpcListener(ioc,
        contextPtr,
        tcp::endpoint{this->serverIp, this->serverPort}));
    rpcListenerPtr->start();
    
    // Run the I/O service on the requested number of threads
    this->threadsVector.reserve(this->threads);
    for(int i = 0; i < this->threads; i++) {
        this->threadsVector.emplace_back(
        [this]
        {
            this->ioc->run();
        });
    }
    //KETO_LOG_DEBUG << "[RpcServer::start] All the threads have been started : " << this->threads;
}

void RpcServer::preStop() {

    KETO_LOG_ERROR << "[RpcServer] begin pre stop";
    rpcListenerPtr->stop();

    KETO_LOG_ERROR << "[RpcServer] wait for session end";
    RpcSessionManager::getInstance()->stop();

    KETO_LOG_ERROR << "[RpcServer] terminate threads";
    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
         iter != this->threadsVector.end(); iter++) {
        iter->join();
    }
    this->threadsVector.clear();
    this->ioc.reset();

    KETO_LOG_ERROR << "[RpcServer] end pre stop";
}

void RpcServer::stop() {

}
    

void RpcServer::setSecret(
        const keto::crypto::SecureVector& secret) {
    this->secret = secret;
}


void RpcServer::setExternalIp(
        const boost::asio::ip::address& ipAddress) {
    if (this->externalIp.is_unspecified()) {
        this->externalIp = ipAddress;
        //std::stringstream sstream;
        //sstream << this->externalIp.to_string() << ":" << this->serverPort;
        //RpcServerSession::getInstance()->addPeer(
        //        keto::server_common::ServerInfo::getInstance()->getAccountHash(),
        //        sstream.str());
    }
}


void RpcServer::setExternalHostname(
        const std::string& externalHostname) {
    if (this->externalHostname.empty()) {
        this->externalHostname = externalHostname;
    }
}

bool RpcServer::hasNetworkState() {
    return this->networkState;
}


void RpcServer::enableNetworkState() {
    keto::proto::RequestNetworkState requestNetworkState;
    // these method are currently not completely implemented but are there as a means to sync with the network state
    // should this later be required.
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RequestNetworkState>(
            keto::server_common::Events::ACTIVATE_NETWORK_STATE_CLIENT,requestNetworkState));
    this->networkState = true;
}

keto::crypto::SecureVector RpcServer::getSecret() {
    return this->secret;
}

keto::event::Event RpcServer::routeTransaction(const keto::event::Event& event) {
    keto::proto::MessageWrapper messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(messageWrapper);
    RpcSessionPtr rpcSessionPtr = RpcSessionManager::getInstance()->getSession(messageWrapper.account_hash());
    if (!rpcSessionPtr) {
        std::stringstream ss;
        ss << "Account [" <<
           messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX)
           << "] is not bound as a peer.";
        BOOST_THROW_EXCEPTION(ClientNotAvailableException(
                ss.str()));
    }
    rpcSessionPtr->routeTransaction(messageWrapper);

    keto::proto::MessageWrapperResponse response;
    response.set_success(true);

    std::stringstream ss;
    ss << "Routed to the peer [" <<
            messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
    response.set_result(ss.str());
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}


keto::event::Event RpcServer::pushBlock(const keto::event::Event& event) {

    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcSessionPtr->pushBlock(keto::server_common::fromEvent<keto::proto::SignedBlockWrapperMessage>(event));
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : unknown cause";
        }
    }
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);

    std::stringstream ss;
    ss << "Block pushed to peers";
    response.set_result(ss.str());

    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event RpcServer::performNetworkSessionReset(const keto::event::Event& event) {
    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcSessionPtr->performNetworkSessionReset();
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : unknown cause";
        }
    }

    keto::proto::MessageWrapperResponse response;
    response.set_success(true);

    std::stringstream ss;
    ss << "Perform network session reset check";
    response.set_result(ss.str());

    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}


keto::event::Event RpcServer::performProtocoCheck(const keto::event::Event& event) {
    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcSessionPtr->performProtocolCheck();
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performProtocoCheck]Failed to perform the protocol check : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::performProtocoCheck]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performProtocoCheck]Failed to perform the protocol check : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performProtocoCheck]Failed to perform the protocol check : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::performProtocoCheck]Failed to perform the protocol check : unknown cause";
        }
    }
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);

    std::stringstream ss;
    ss << "Perform protocol check";
    response.set_result(ss.str());

    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event RpcServer::performConsensusHeartbeat(const keto::event::Event& event) {
    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcSessionPtr->performNetworkHeartbeat(
                    keto::server_common::fromEvent<keto::proto::ProtocolHeartbeatMessage>(event));
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : unknown cause";
        }
    }

    keto::proto::MessageWrapperResponse response;
    response.set_success(true);

    std::stringstream ss;
    ss << "Perform network heartbeat";
    response.set_result(ss.str());

    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event RpcServer::electBlockProducer(const keto::event::Event& event) {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    keto::election_common::ElectionMessageProtoHelper electionMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionMessage>(event));

    std::vector<RpcSessionPtr> sessions = RpcSessionManager::getInstance()->getActiveSessions();
    for (int index = 0; (index < keto::server_common::Constants::ELECTION::ELECTOR_COUNT) && (sessions.size()); index++) {
        RpcSessionPtr rpcSessionPtr;
        if (sessions.size() > 1) {
            std::uniform_int_distribution<int> distribution(0, sessions.size() - 1);
            distribution(stdGenerator);
            int pos = distribution(stdGenerator);
            rpcSessionPtr = sessions[pos];
            sessions.erase(sessions.begin() + pos);
        } else {
            rpcSessionPtr = sessions[0];
            sessions.clear();
        }
        try {
            if (rpcSessionPtr->electBlockProducer()) {
                electionMessageProtoHelper.addAccount(keto::asn1::HashHelper(rpcSessionPtr->getAccount()));
            }

        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::electBlockProducer]Failed to perform the elect block : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::electBlockProducer]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::electBlockProducer]Failed to perform the elect block : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::electBlockProducer]Failed to perform the elect block : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::electBlockProducer]Failed to perform the elect block : unknown cause";
        }

    }

    return keto::server_common::toEvent<keto::proto::ElectionMessage>(electionMessageProtoHelper);
}


keto::event::Event RpcServer::activatePeers(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    serverActive = rpcPeerHelper.isActive();
    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcSessionPtr->activatePeer(rpcPeerHelper);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : unknown cause";
        }
    }

    return event;
}

keto::event::Event RpcServer::requestNetworkState(const keto::event::Event& event) {
    KETO_LOG_INFO << "Request network state from client";
    this->networkState = false;
    return event;
}

keto::event::Event RpcServer::activateNetworkState(const keto::event::Event& event) {
    if (!this->networkState) {
        KETO_LOG_INFO << "Activte network state from client";
        this->networkState = true;
    }
    return event;
}

keto::event::Event RpcServer::requestBlockSync(const keto::event::Event& event) {
    keto::proto::SignedBlockBatchRequest request = keto::server_common::fromEvent<keto::proto::SignedBlockBatchRequest>(event);
    std::vector<RpcSessionPtr> rpcSessions = RpcSessionManager::getInstance()->getActiveSessions();
    if (!rpcSessions.empty()) {
        RpcSessionPtr lastTouchRpcSessionPtr = rpcSessions[0];
        for (int index = 1; index < rpcSessions.size(); index++) {
            RpcSessionPtr currentSessionPtr = rpcSessions[index];
            if (lastTouchRpcSessionPtr->getLastBlockTouch() > currentSessionPtr->getLastBlockTouch()) {
                lastTouchRpcSessionPtr = currentSessionPtr;
            }
        }
        try {
            lastTouchRpcSessionPtr->requestBlockSync(request);
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Cause : " << boost::diagnostic_information(ex, true);
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : "
                           << boost::diagnostic_information(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : unknown cause";
        }
    } else {
        // this will force a call to the RPC server to sync
        KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Fall back failed no downstream active nodes";
    }

    return event;
}

bool RpcServer::isServerActive() {
    return (this->serverActive || keto::server_common::ServerInfo::getInstance()->isMaster());
}

std::string RpcServer::getExternalPeerInfo() {
    std::stringstream sstream;
    std::string hostname = this->externalIp.to_string();
    if (!externalHostname.empty()) {
        hostname = externalHostname;
    }

    sstream << hostname << ":" << this->serverPort;
    return sstream.str();
}

bool RpcServer::isTerminated() {
    std::lock_guard<std::mutex> guard(classMutex);
    return terminated;
}

void RpcServer::terminate() {
    std::lock_guard<std::mutex> guard(classMutex);
    this->terminated = true;
}

keto::event::Event RpcServer::electBlockProducerPublish(const keto::event::Event& event) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));
    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcSessionPtr->electBlockProducerPublish(electionPublishTangleAccountProtoHelper);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::performConsensusHeartbeat]Failed to perform the consensus heartbeat : unknown cause";
        }
    }

    return event;
}


keto::event::Event RpcServer::electBlockProducerConfirmation(const keto::event::Event& event) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));
    for (RpcSessionPtr rpcSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
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

    return event;
}


}
}
