//
// Created by Brett Chaldecott on 2021/07/20.
//

#include "keto/rpc_server/RpcReceiveQueue.hpp"
#include "keto/rpc_server/RpcSessionManager.hpp"
#include "keto/rpc_server/RpcServer.hpp"
#include "keto/rpc_server/Exception.hpp"
#include "keto/rpc_server/Constants.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/rpc_protocol/PeerRequestHelper.hpp"
#include "keto/rpc_protocol/PeerResponseHelper.hpp"
#include "keto/router_utils/RpcPeerHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperResponseHelper.hpp"
#include "keto/election_common/PublishedElectionInformationHelper.hpp"
#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"

#include "keto/transaction/Transaction.hpp"
#include "keto/server_common/TransactionHelper.hpp"

#include "keto/software_consensus/ModuleConsensusValidationMessageHelper.hpp"
#include "keto/software_consensus/ConsensusStateManager.hpp"

#include <botan/hex.h>
#include <botan/base64.h>

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/rpc_server/RpcServerSession.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

std::string RpcReceiveQueue::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcReceiveQueue::RpcReceiveQueue(int sessionId) : sessionId(sessionId), active(true), clientActive(false), registered(false), aborted(false) {
    this->generatorPtr = std::shared_ptr<Botan::AutoSeeded_RNG>(new Botan::AutoSeeded_RNG());
}

RpcReceiveQueue::~RpcReceiveQueue() {

}

void RpcReceiveQueue::start(const RpcSendQueuePtr& rpcSendQueuePtr) {
    this->rpcSendQueuePtr = rpcSendQueuePtr;
    queueThreadPtr = std::shared_ptr<std::thread>(new std::thread(
            [this]
            {
                this->run();
            }));
}

void RpcReceiveQueue::preStop() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!active) {
        return;
    }
    active = false;
    this->stateCondition.notify_all();
}

void RpcReceiveQueue::stop() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->readQueue.clear();
    this->stateCondition.notify_all();
}

void RpcReceiveQueue::abort() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (aborted) {
        return;
    }
    aborted = true;
    this->readQueue.clear();
    this->stateCondition.notify_all();
}

void RpcReceiveQueue::join() {
    KETO_LOG_INFO << "[RpcReceiveQueue::join][" << sessionId << "] wait for join";
    queueThreadPtr->join();
    KETO_LOG_INFO << "[RpcReceiveQueue::join][" << sessionId << "] after join";
}

bool RpcReceiveQueue::clientIsActive() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return clientActive;
}

bool RpcReceiveQueue::isActive() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return active;
}

bool RpcReceiveQueue::isRegistered() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return registered;
}

void RpcReceiveQueue::pushEntry(const std::string& command, const std::string& payload) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!active || aborted) {
        return;
    }
    this->readQueue.push_back(RpcReadQueueEntryPtr(new RpcReadQueueEntry(command,payload)));
    this->stateCondition.notify_all();
}

void RpcReceiveQueue::performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage) {
    this->electionResultCache.heartBeat(protocolHeartbeatMessage);
}

long RpcReceiveQueue::getLastBlockTouch() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->lastBlockTouch;
}

long RpcReceiveQueue::blockTouch() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->lastBlockTouch = time(0);
}

std::string RpcReceiveQueue::getAccountHash() {
    if (serverHelloProtoHelperPtr) {
        return keto::server_common::VectorUtils().copyVectorToString(
                serverHelloProtoHelperPtr->getAccountHash());
    }
    return std::string();
}

std::string RpcReceiveQueue::getAccountHashStr() {
    if (serverHelloProtoHelperPtr) {
        return  serverHelloProtoHelperPtr->getAccountHashStr();
    }
    return std::string();
}

void RpcReceiveQueue::setClientActive(bool clientActive) {
    this->clientActive = clientActive;
}

std::string RpcReceiveQueue::getAccount() {
    if (serverHelloProtoHelperPtr) {
        return Botan::hex_encode(
                serverHelloProtoHelperPtr->getAccountHash());
    }
    return "";
}

void RpcReceiveQueue::run() {
    RpcReadQueueEntryPtr rpcReadQueueEntryPtr;
    while(rpcReadQueueEntryPtr = popEntry()) {
        processEntry(rpcReadQueueEntryPtr);
    }
}

RpcReadQueueEntryPtr RpcReceiveQueue::popEntry() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    while(active && !aborted) {
        if (!this->readQueue.empty()) {
            RpcReadQueueEntryPtr entry = this->readQueue.front();
            this->readQueue.pop_front();
            return entry;
        }
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_SERVER_QUEUE_DELAY));
    }
    return RpcReadQueueEntryPtr();
}

void RpcReceiveQueue::processEntry(const RpcReadQueueEntryPtr& entry) {
    std::string command = entry->getCommand();
    std::string payload = entry->getPayload();

    try {
        //KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] process the command : " << command;
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) == 0) {
            handleBlockSyncRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,payload);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_REQUEST) == 0) {
            // transaction bound method
            handleMissingBlockSyncRequest(command, payload);
        } else {
            keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
            if (command == keto::server_common::Constants::RPC_COMMANDS::CLOSE) {
                this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSE,keto::server_common::Constants::RPC_COMMANDS::CLOSE);
            } else if (keto::software_consensus::ConsensusStateManager::getInstance()->getState()
                != keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
                KETO_LOG_INFO << "[RpcServer] This peer is currently not configured reconnection required : "
                              << keto::software_consensus::ConsensusStateManager::getInstance()->getState();
                handleRetryResponse(command);
            } else {
                if (command == keto::server_common::Constants::RPC_COMMANDS::HELLO) {
                    handleHello(command, payload);
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " Said hello";
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) {
                    handleHelloConsensus(command, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::PEERS) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " requested peers";
                    handlePeer(command, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::REGISTER) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " register";
                    handleRegister(keto::server_common::Constants::RPC_COMMANDS::REGISTER, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " push rpc peers";
                    handlePushRpcPeers(keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::ACTIVATE) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " activate";
                    handleActivate(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] handle a transaction";
                    handleTransaction(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION, payload);
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Transaction processing complete";
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) {
                    handleTransactionProcessed(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED,payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) {

                } else if (command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS) {
                    handleRequestNetworkSessionKeys(
                            keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS) {
                    handleRequestMasterNetworkKeys(
                            keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS) {
                    handleRequestNetworkKeys(
                            keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS,
                            payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES) {
                    handleRequestNetworkFees(
                            keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES,payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE) {
                    handleClientNetworkComplete(
                            keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK) {
                    handleBlockPush(keto::server_common::Constants::RPC_COMMANDS::BLOCK, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) {
                    handleBlockSyncResponse(command, payload);
                } else if (command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE) {
                    handleProtocolCheckResponse(
                            keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST) == 0) {
                    handleElectionRequest(
                            keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                            payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE) ==
                           0) {
                    handleElectionResponse(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE,
                                           payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH) == 0) {
                    handleElectionPublish(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                                          payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION) ==
                           0) {
                    handleElectionConfirmation(
                            keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                            payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS) ==
                           0) {
                    handleResponseNetworkStatus(command, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_RESPONSE) == 0) {
                    handleMissingBlockSyncResponse(command, payload);
                }
            }
            transactionPtr->commit();
        }
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "] on the server [keto::common::Exception]: " << boost::diagnostic_information(ex,true);
        handleRetryResponse(command);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "]on the server [boost::exception]: " << boost::diagnostic_information(ex,true);
        handleRetryResponse(command);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "] on the server [std::exception]: " << ex.what();
        handleRetryResponse(command);
    } catch (...) {
        KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "] on the server [...]: unknown";
        handleRetryResponse(command);
    }


}

void RpcReceiveQueue::handleBlockSyncRequest(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] handle the block sync request : " << command;
    keto::proto::SignedBlockBatchRequest signedBlockBatchRequest;
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload));
    signedBlockBatchRequest.ParseFromString(rpcVector);
    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage =
            keto::server_common::fromEvent<keto::proto::SignedBlockBatchMessage>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                            keto::server_common::Events::BLOCK_DB_REQUEST_BLOCK_SYNC,signedBlockBatchRequest)));

    std::string result = signedBlockBatchMessage.SerializeAsString();
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] Setup the block sync reply";
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] The block data returned [" << result.size() << "]";
}

void RpcReceiveQueue::handleHello(const std::string& command, const std::string& payload) {
    std::string bytes = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload));
    this->serverHelloProtoHelperPtr =
            std::shared_ptr<keto::rpc_protocol::ServerHelloProtoHelper>(
                    new keto::rpc_protocol::ServerHelloProtoHelper(bytes));

    std::stringstream ss;
    ss << Botan::hex_encode(RpcServer::getInstance()->getSecret())
       << " " << Botan::hex_encode(this->generateSession());

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,
                                     ss.str());
}

void RpcReceiveQueue::handleRetryResponse(const std::string& command) {
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_RETRY,command);
}

void RpcReceiveQueue::handleHelloConsensus(const std::string& command, const std::string& payload) {
    keto::proto::ConsensusMessage consensusMessage;
    std::string binString = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload,true));
    consensusMessage.ParseFromString(binString);
    keto::proto::ModuleConsensusValidationMessage moduleConsensusValidationMessage =
            keto::server_common::fromEvent<keto::proto::ModuleConsensusValidationMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ConsensusMessage>(
                                    keto::server_common::Events::VALIDATE_SOFTWARE_CONSENSUS_MESSAGE,consensusMessage)));
    keto::software_consensus::ModuleConsensusValidationMessageHelper moduleConsensusValidationMessageHelper(
            moduleConsensusValidationMessage);
    std::stringstream ss;
    if (moduleConsensusValidationMessageHelper.isValid()) {
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED,keto::server_common::Constants::RPC_COMMANDS::ACCEPTED);
        //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was accepted";
    } else {
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY,keto::server_common::Constants::RPC_COMMANDS::GO_AWAY);
        //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was rejected from network";
    }
}


void RpcReceiveQueue::handlePeer(const std::string& command, const std::string& payload) {
    // retrieve the peer information
    std::string binString = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload,true));
    keto::proto::PeerRequest peerRequest;
    peerRequest.ParseFromString(binString);
    keto::rpc_protocol::PeerRequestHelper peerRequestHelper(peerRequest);
    if (!peerRequestHelper.getHostname().empty()) {
        this->remoteHostname = peerRequestHelper.getHostname();
    }

    //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " get the peers for a client";
    RpcServer::getInstance()->setExternalIp(this->localAddress);
    RpcServer::getInstance()->setExternalHostname(this->localHostname);
    std::vector<std::string> urls;
    keto::rpc_protocol::PeerResponseHelper peerResponseHelper;

    // get the list of peers but ignore the current account
    std::stringstream str;
    if (!this->remoteHostname.empty()) {
        str << this->remoteHostname << ":" << Constants::DEFAULT_PORT_NUMBER;
    } else {
        str << this->remoteAddress << ":" << Constants::DEFAULT_PORT_NUMBER;
    }

    std::vector<std::string> peers = RpcServerSession::getInstance()->handlePeers(this->serverHelloProtoHelperPtr->getAccountHash(),str.str());
    if (peers.size() < Constants::MIN_PEERS) {
        // exclude external peers that match this one
        std::string externalPeerInfo = RpcServer::getInstance()->getExternalPeerInfo();
        if (externalPeerInfo != str.str()) {
            peers.push_back(externalPeerInfo);
        }
    }
    peerResponseHelper.addPeers(peers);

    // return the list of peers
    std::string result;
    peerResponseHelper.operator keto::proto::PeerResponse().SerializePartialToString(&result);
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PEERS,Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleRegister(const std::string& command, const std::string& payload) {

    // split and extract the peer information to be added
    keto::server_common::StringVector parts = keto::server_common::StringUtils(payload).tokenize("#");

    // add the peered host assuming this method has been called to reregister
    std::string binString = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(parts[0],true));
    keto::proto::PeerRequest peerRequest;
    peerRequest.ParseFromString(binString);
    keto::rpc_protocol::PeerRequestHelper peerRequestHelper(peerRequest);
    if (!peerRequestHelper.getHostname().empty()) {
        this->remoteHostname = peerRequestHelper.getHostname();
    }
    //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " get the peers for a client";
    RpcServer::getInstance()->setExternalIp(this->localAddress);
    RpcServer::getInstance()->setExternalHostname(this->localHostname);
    std::stringstream str;
    if (!this->remoteHostname.empty()) {
        str << this->remoteHostname << ":" << Constants::DEFAULT_PORT_NUMBER;
    } else {
        str << this->remoteAddress << ":" << Constants::DEFAULT_PORT_NUMBER;
    }
    RpcServerSession::getInstance()->addPeer(this->serverHelloProtoHelperPtr->getAccountHash(),str.str());

    // register the client peer with the account
    std::string rpcPeerBytes = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(parts[1]));
    keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcPeerBytes);
    keto::proto::RpcPeer rpcPeer = (keto::proto::RpcPeer)rpcPeerHelper;

    rpcPeer = keto::server_common::fromEvent<keto::proto::RpcPeer>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::RpcPeer>(
                            keto::server_common::Events::REGISTER_RPC_PEER_CLIENT,rpcPeer)));

    // registered
    this->registered = true;
    this->clientActive = rpcPeerHelper.isActive();

    // set up the peer information
    keto::router_utils::RpcPeerHelper serverPeerHelper;
    serverPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    serverPeerHelper.setActive(RpcServer::getInstance()->isServerActive());
    std::string serverPeerStr = serverPeerHelper;

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REGISTER,
                                     Botan::hex_encode((uint8_t*)serverPeerStr.data(),serverPeerStr.size(),true));
}

void RpcReceiveQueue::handlePushRpcPeers(const std::string& command, const std::string& payload) {
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload));
    keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);


    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ROUTER_QUERY::PROCESS_PUSH_RPC_PEER,rpcPeerHelper));
}

void RpcReceiveQueue::handleActivate(const std::string& command, const std::string& payload) {
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload));
    keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleActivate] activate the node : " << rpcPeerHelper.isActive();
    this->clientActive = rpcPeerHelper.isActive();

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ACTIVATE_RPC_PEER,rpcPeerHelper));
}

void RpcReceiveQueue::handleTransaction(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleTransaction] handle transaction";
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));
    messageWrapperProtoHelper.setSessionHash(
            keto::server_common::VectorUtils().copyVectorToString(
                    serverHelloProtoHelperPtr->getAccountHash()));

    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                            keto::server_common::Events::ROUTE_MESSAGE,messageWrapper)));

    std::string result = messageWrapperResponse.SerializeAsString();

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));

}

void RpcReceiveQueue::handleTransactionProcessed(const std::string& command, const std::string& payload) {

    keto::proto::MessageWrapperResponse messageWrapperResponse;
    messageWrapperResponse.ParseFromString(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

}

void RpcReceiveQueue::handleRequestNetworkSessionKeys(const std::string& command, const std::string& payload) {
    keto::proto::NetworkKeysWrapper networkKeysWrapper;
    networkKeysWrapper =
            keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::GET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

    std::string result = networkKeysWrapper.SerializeAsString();

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));

}

void RpcReceiveQueue::handleRequestMasterNetworkKeys(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestMasterNetworkKeys] << request the master network keys :" << command;

    keto::proto::NetworkKeysWrapper networkKeysWrapper;
    networkKeysWrapper =
            keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::GET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

    std::string result = networkKeysWrapper.SerializeAsString();

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleRequestNetworkKeys(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] << request the network key :" << command;

    keto::proto::NetworkKeysWrapper networkKeysWrapper;
    networkKeysWrapper =
            keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::GET_NETWORK_KEYS,networkKeysWrapper)));

    std::string result = networkKeysWrapper.SerializeAsString();
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));

}

void RpcReceiveQueue::handleRequestNetworkFees(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkFees] handle request network fees";
    keto::proto::FeeInfoMsg feeInfoMsg;
    feeInfoMsg =
            keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::FeeInfoMsg>(
                            keto::server_common::Events::NETWORK_FEE_INFO::GET_NETWORK_FEE,feeInfoMsg)));

    std::string result = feeInfoMsg.SerializeAsString();
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleClientNetworkComplete(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleClientNetworkComplete] The client has completed networking";
    std::stringstream ss;
    // At present the network status is not required
    if (!RpcServer::getInstance()->hasNetworkState()) {
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS,
                                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS);
    } else {
        // respond with the published election information
        keto::proto::PublishedElectionInformation publishedElectionInformation;
        publishedElectionInformation =
                keto::server_common::fromEvent<keto::proto::PublishedElectionInformation>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
                                keto::server_common::Events::ROUTER_QUERY::GET_PUBLISHED_ELECTION_INFO,publishedElectionInformation)));

        std::string result = publishedElectionInformation.SerializeAsString();
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS,
                                         Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
    }
}

void RpcReceiveQueue::handleBlockPush(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockPush] handle block push";
    if (!clientIsActive()) {
        setClientActive(true);
    }
    blockTouch();
    keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
    signedBlockWrapperMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));
    KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleBlockPush] persist bock";
    keto::transaction_common::MessageWrapperResponseHelper messageWrapperResponseHelper(
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
                            keto::server_common::Events::BLOCK_PERSIST_MESSAGE_SERVER,signedBlockWrapperMessage))));
    if (!messageWrapperResponseHelper.isSuccess() && !messageWrapperResponseHelper.getMsg().empty()) {
        keto::proto::SignedBlockBatchRequest signedBlockBatchRequest;
        signedBlockBatchRequest.ParseFromString(messageWrapperResponseHelper.getBinaryMsg());
        KETO_LOG_INFO << "[RpcReceiveQueue::handleBlockPush] Chain is out of sync test serialization :" <<
            signedBlockBatchRequest.tangle_hashes_size();
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_REQUEST,
                                         messageWrapperResponseHelper.getMsg());
    }
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockPush] pushed the block";

}

void RpcReceiveQueue::handleBlockSyncResponse(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncResponse] handle the block sync request : " << command;
    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));
    if (signedBlockBatchMessage.partial_result()) {
        setClientActive(false);
    }
    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                            keto::server_common::Events::BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage)));

    std::string result = messageWrapperResponse.SerializeAsString();
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_PROCESSED,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleMissingBlockSyncRequest(const std::string& command, const std::string& payload) {
    keto::proto::SignedBlockBatchRequest signedBlockBatchRequest;
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload));
    signedBlockBatchRequest.ParseFromString(rpcVector);

    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage =
            keto::server_common::fromEvent<keto::proto::SignedBlockBatchMessage>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                            keto::server_common::Events::MISSING_BLOCK_DB_REQUEST_BLOCK_SYNC,signedBlockBatchRequest)));

    std::string result = signedBlockBatchMessage.SerializeAsString();
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_RESPONSE, Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleMissingBlockSyncResponse(const std::string& command, const std::string& payload) {
    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));
    if (signedBlockBatchMessage.partial_result()) {
        setClientActive(false);
    }
    keto::transaction_common::MessageWrapperResponseHelper messageWrapperResponseHelper(
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                            keto::server_common::Events::MISSING_BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage))));

    if (!messageWrapperResponseHelper.isSuccess() && !messageWrapperResponseHelper.getMsg().empty()) {
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_REQUEST,
                                         messageWrapperResponseHelper.getMsg());
    }
}

void RpcReceiveQueue::handleProtocolCheckResponse(const std::string& command, const std::string& payload) {
    keto::proto::ConsensusMessage consensusMessage;
    std::string binString = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload,true));
    consensusMessage.ParseFromString(binString);
    keto::proto::ModuleConsensusValidationMessage moduleConsensusValidationMessage =
            keto::server_common::fromEvent<keto::proto::ModuleConsensusValidationMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ConsensusMessage>(
                                    keto::server_common::Events::VALIDATE_SOFTWARE_CONSENSUS_MESSAGE,consensusMessage)));
    keto::software_consensus::ModuleConsensusValidationMessageHelper moduleConsensusValidationMessageHelper(
            moduleConsensusValidationMessage);
    if (moduleConsensusValidationMessageHelper.isValid()) {
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT,
                                         keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT);
    } else {
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY,
                                         keto::server_common::Constants::RPC_COMMANDS::GO_AWAY);
    }
}

void RpcReceiveQueue::handleElectionRequest(const std::string& command, const std::string& payload) {
    //KETO_LOG_DEBUG << "[RpcServer::handleElectionRequest] handle the election request : " << command;
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionResultMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ElectionPeerMessage>(
                                    keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_REQUEST,electionPeerMessageProtoHelper))));

    std::string result = electionResultMessageProtoHelper;
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleElectionResponse(const std::string& command, const std::string& payload) {
    KETO_LOG_INFO << getAccount() << "[handleElectionResponse]: process election response";
    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::ElectionResultMessage>(
                    keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE,electionResultMessageProtoHelper));
    KETO_LOG_INFO << getAccount() << "[handleElectionResponse]: processed election response";
}

void RpcReceiveQueue::handleElectionPublish(const std::string& command, const std::string& message) {
    //KETO_LOG_DEBUG << "[handleElectionPublish] handle the election";
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper)) {
        //KETO_LOG_DEBUG << "[handleElectionPublish] handle the election";
        return;
    }

    //KETO_LOG_DEBUG << "[handleElectionPublish] publish the election";
    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_PUBLISH).
            publish(electionPublishTangleAccountProtoHelper);
}

void RpcReceiveQueue::handleElectionConfirmation(const std::string& command, const std::string& message) {
    //KETO_LOG_DEBUG << "[handleElectionConfirmation] confirm the election";
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        //KETO_LOG_DEBUG << "[handleElectionConfirmation] ignore the confirmation election";
        return;
    }

    //KETO_LOG_DEBUG << "[handleElectionConfirmation] process the confirmation";
    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_CONFIRMATION).
            confirmation(electionConfirmationHelper);
}

void RpcReceiveQueue::handleResponseNetworkStatus(const std::string& command, const std::string& payload) {
    keto::election_common::PublishedElectionInformationHelper publishedElectionInformationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    KETO_LOG_ERROR << "[RpcServer][" << getAccount() << "][handleResponseNetworkStatus] set the network status";
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
            keto::server_common::Events::ROUTER_QUERY::SET_PUBLISHED_ELECTION_INFO,publishedElectionInformationHelper));
    //RpcSessionManager::getInstance()->enableNetworkState();
    KETO_LOG_ERROR << "[RpcServer][" << getAccount() << "][handleResponseNetworkStatus] setup network status";


}

keto::crypto::SecureVector RpcReceiveQueue::generateSession() {
    return this->sessionIdentifier = this->generatorPtr->random_vec(Constants::SESSION_ID_LENGTH);
}




}
}

