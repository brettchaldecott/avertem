//
// Created by Brett Chaldecott on 2021/08/14.
//

#include <botan/hex.h>
#include <boost/asio/ip/host_name.hpp>

#include "keto/rpc_client/RpcReceiveQueue.hpp"
#include "keto/rpc_client/RpcSendQueue.hpp"
#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/RpcClient.hpp"
#include "keto/rpc_client/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/TransactionHelper.hpp"

#include "keto/transaction/Transaction.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperResponseHelper.hpp"

#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "keto/rpc_protocol/PeerResponseHelper.hpp"
#include "keto/rpc_protocol/PeerRequestHelper.hpp"
#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"
#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"


namespace keto {
namespace rpc_client {

std::string RpcReceiveQueue::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcReceiveQueue::RpcReceiveQueue(int sessionId, const RpcPeer& rpcPeer) :
    sessionId(sessionId), active(true), clientActive(false), registered(false), aborted(false), rpcPeer(rpcPeer) {

    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::rpc_client::PrivateKeyNotConfiguredException());
    }
    std::string privateKeyPath =
            config->getVariablesMap()[Constants::PRIVATE_KEY].as<std::string>();
    if (!config->getVariablesMap().count(Constants::PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::rpc_client::PrivateKeyNotConfiguredException());
    }
    std::string publicKeyPath =
            config->getVariablesMap()[Constants::PUBLIC_KEY].as<std::string>();
    keyLoaderPtr = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
                                                             publicKeyPath);

    // setup the external hostname information
    if (config->getVariablesMap().count(Constants::EXTERNAL_HOSTNAME)) {
        this->externalHostname =
                config->getVariablesMap()[Constants::EXTERNAL_HOSTNAME].as<std::string>();
    } else {
        this->externalHostname = boost::asio::ip::host_name();
    }

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
    KETO_LOG_INFO << getHost() << "[RpcReceiveQueue::preStop] pre stop begin";
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!active) {
        return;
    }
    active = false;
    this->stateCondition.notify_all();
    KETO_LOG_INFO << getHost() << "[RpcReceiveQueue::preStop] pre stop end";
}

void RpcReceiveQueue::stop() {
    KETO_LOG_INFO << getHost() << "[RpcReceiveQueue::stop] stop begin";
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    active = false;
    this->readQueue.clear();
    this->stateCondition.notify_all();
    KETO_LOG_INFO << getHost() << "[RpcReceiveQueue::stop] stop end";
}

void RpcReceiveQueue::abort() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (aborted) {
        return;
    }
    aborted = true;
    this->stateCondition.notify_all();
}

void RpcReceiveQueue::join() {
    KETO_LOG_INFO << getHost() << "[RpcReceiveQueue::join] wait join";
    queueThreadPtr->join();
    KETO_LOG_INFO << getHost() << "[RpcReceiveQueue::join] join finished";
}

bool RpcReceiveQueue::clientIsActive() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return clientActive;
}

void RpcReceiveQueue::setClientActive(bool clientActive) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->clientActive = clientActive;
}

bool RpcReceiveQueue::isActive() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return active;
}
bool RpcReceiveQueue::isRegistered() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return registered;
}

bool RpcReceiveQueue::containsPublishAccount(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper);
}

bool RpcReceiveQueue::containsConfirmationAccount(const keto::asn1::HashHelper& hashHelper) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return this->electionResultCache.containsConfirmationAccount(hashHelper);
}

void RpcReceiveQueue::pushEntry(const std::string& command, const std::string& payload, const std::string& misc) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!active || aborted) {
        return;
    }
    this->readQueue.push_back(RpcReadQueueEntryPtr(new RpcReadQueueEntry(command,payload, misc)));
    this->stateCondition.notify_all();
}

// block touch methods
long RpcReceiveQueue::getLastBlockTouch() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->lastBlockTouch;
}

long RpcReceiveQueue::blockTouch() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->lastBlockTouch = time(0);
}

int RpcReceiveQueue::getSessionId() {
    return this->sessionId;
}

std::string RpcReceiveQueue::getHost() {
    return this->rpcPeer.getHost();
}

std::string RpcReceiveQueue::getAccountHash() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return accountHash;
}

std::string RpcReceiveQueue::getAccountHashStr() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return Botan::hex_encode((uint8_t*)accountHash.data(),accountHash.size(),true);
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
                Constants::DEFAULT_RPC_CLIENT_QUEUE_DELAY));
    }
    return RpcReadQueueEntryPtr();
}

void RpcReceiveQueue::processEntry(const RpcReadQueueEntryPtr& entry) {
    std::string command = entry->getCommand();
    std::string payload = entry->getPayload();
    std::string misc = entry->getMisc();

    try {
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) == 0) {
            // none transaction bound method
            handleBlockSyncRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,payload);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_REQUEST) == 0) {
            // none transaction bound method
            handleMissingBlockSyncRequest(command, payload);
        } else {
            // transaction scoped methods
            keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();

            // Close the WebSocket connection
            if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO) == 0) {
                handleHelloRequest(keto::server_common::Constants::RPC_COMMANDS::HELLO,
                                   keto::server_common::Constants::RPC_COMMANDS::HELLO);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
                helloConsensusResponse(command, payload, misc);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY) == 0) {
                //closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE, stringVector[1]);
                handleRetryResponse(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED) == 0) {
                helloAcceptedResponse(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED,payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
                // peer response requires this session is shut down
                handlePeerResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REGISTER) == 0) {
                handleRegisterResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE) == 0) {
                handleActivatePeer(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
                handleTransaction(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK) == 0) {
                handleBlock(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED) == 0) {
                // do nothing
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION) == 0) {
                consensusSessionResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {
                consensusResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS) ==
            0) {
                handleRequestNetworkSessionKeysResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS) ==
            0) {
                handleRequestNetworkMasterKeyResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS) == 0) {
                handleRequestNetworkKeysResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) == 0) {
                handleRequestNetworkFeesResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
                closeResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_RETRY) == 0) {
                handleRetryResponse(payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) == 0) {
                handleBlockSyncResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_RESPONSE) == 0) {
                handleMissingBlockSyncResponse(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST) == 0) {
                handleProtocolCheckRequest(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT) == 0) {
                handleProtocolCheckAccept(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_HEARTBEAT) == 0) {
                handleProtocolHeartbeat(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST) == 0) {
                handleElectionRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                                                payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE) == 0) {
                handleElectionResponse(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH) == 0) {
                handleElectionPublish(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION) == 0) {
                handleElectionConfirmation(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS) == 0) {
                handleRequestNetworkStatus(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS,
                        payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS) == 0) {
                handleResponseNetworkStatus(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS,
                                            payload);
            }
            transactionPtr->commit();
        }
    } catch (keto::common::Exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->getSessionId() << "]" << command
        << " : Failed to process because : " << boost::diagnostic_information_what(ex, true);
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry] cause : " << ex.what();
        handleInternalException(command,ex.what());
    } catch (boost::exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->getSessionId() << "]" << command
        << " : Failed to process because : " << boost::diagnostic_information_what(ex, true);
        handleInternalException(command);
    } catch (std::exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->getSessionId() << "]" << command
        << " : Failed process the request : " << ex.what();
        handleInternalException(command);
    } catch (...) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->getSessionId() << "]" << command
        << " : Failed process the request" << std::endl;
        handleInternalException(command);
    }
}


void RpcReceiveQueue::handleBlockSyncRequest(const std::string& command, const std::string& payload) {
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
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE, Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    keto::asn1::HashHelper initHashHelper(initHash,keto::common::StringEncoding::HEX);
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    // guarantee order of consensus handling to prevent sessions from getting incorrectly setup
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    std::vector<uint8_t> result = buildConsensus(initHashHelper);
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleHelloRequest(const std::string& command, const std::string& payload) {
    //KETO_LOG_INFO << "[RpcSession::handleHelloRequest] Received Queue [" << command << "]";
    std::vector<uint8_t> result = buildHeloMessage();
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::HELLO,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleRetryResponse(const std::string& command) {

    std::string result;
    //KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Processing failed for the command : " << command;
    if (command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS ||
        command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS  ||
        command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS ||
        command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES ||
        command == keto::server_common::Constants::RPC_COMMANDS::HELLO ||
        command == keto::server_common::Constants::RPC_COMMANDS::ACCEPTED ||
        command == keto::server_common::Constants::RPC_COMMANDS::GO_AWAY ||
        command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT ||
        command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS ||
        command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION ||
        command == keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS ||
        command == keto::server_common::Constants::RPC_COMMANDS::PEERS ||
        command == keto::server_common::Constants::RPC_COMMANDS::REGISTER ||
        command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST ||
        command == keto::server_common::Constants::RPC_COMMANDS::BLOCK) {
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse][" << this->getHost() << "] close connect to reconnect";
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSE,
                                         keto::server_common::Constants::RPC_COMMANDS::CLOSE);
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) {
        // this indicates the up stream server is currently out of sync and cannot be relied upon we therefore
        // need to use an alternative and mark this one as inactive until it is activated.
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse][" << this->getHost() << "] Block sync requires a retry reschedule";
        if (this->clientIsActive()) {
            this->setClientActive(false);
        }
        // reschedule the block sync retry
        keto::proto::MessageWrapper messageWrapper;
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                keto::server_common::Events::BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY,messageWrapper));
    } else {
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Ignore as no retry is required";
        KETO_LOG_INFO << this->getHost() << ": Setup connection for read : " << command << std::endl;
    }
}


void RpcReceiveQueue::helloAcceptedResponse(const std::string& command, const std::string& payload) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    if (!this->rpcPeer.getPeered()) {
        return handlePeerRequest(command, payload);
    }

    // notify the accepted inorder to set the network keys
    KETO_LOG_INFO << "[RpcSession::helloAcceptedResponse] handle network session accepted";
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyAccepted();

    if (this->isRegistered()) {
        return handleRequestNetworkSessionKeys(command, payload);
    } else {
        return handleRegisterRequest(command, payload);
    }
}

void RpcReceiveQueue::handlePeerRequest(const std::string& command, const std::string& payload) {
    keto::rpc_protocol::PeerRequestHelper peerRequestHelper;

    peerRequestHelper.addAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    peerRequestHelper.setHostname(this->externalHostname);

    keto::proto::PeerRequest peerRequest = peerRequestHelper;
    std::string peerRequestValue;
    peerRequest.SerializePartialToString(&peerRequestValue);

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PEERS,
                                     Botan::hex_encode((uint8_t*)peerRequestValue.data(),peerRequestValue.size(),true));
}

void RpcReceiveQueue::handleRequestNetworkSessionKeys(const std::string& command, const std::string& payload) {
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}

void RpcReceiveQueue::handleRegisterRequest(const std::string& command, const std::string& payload) {

    // peer request helper
    keto::rpc_protocol::PeerRequestHelper peerRequestHelper;
    peerRequestHelper.addAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    peerRequestHelper.setHostname(this->externalHostname);
    keto::proto::PeerRequest peerRequest = peerRequestHelper;
    std::string peerRequestValue;
    peerRequest.SerializePartialToString(&peerRequestValue);

    // notify the accepted
    keto::router_utils::RpcPeerHelper rpcPeerHelper;
    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    rpcPeerHelper.setActive(RpcClient::getInstance()->isActivated());
    keto::proto::RpcPeer rpcPeer = rpcPeerHelper;
    std::string rpcValue;
    rpcPeer.SerializePartialToString(&rpcValue);

    // setup the registration response including the mising peer request and rpc information
    std::stringstream sstream;
    sstream <<  Botan::hex_encode((uint8_t*)peerRequestValue.data(),peerRequestValue.size(),true) << "#" <<
    Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true);

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REGISTER, sstream.str());
}

void RpcReceiveQueue::handlePeerResponse(const std::string& command, const std::string& payload) {
    std::string response = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload,true));
    keto::rpc_protocol::PeerResponseHelper peerResponseHelper(response);

    // Read a message into our buffer
    RpcClient::getInstance()->setPeers(peerResponseHelper.getPeers());
    //KETO_LOG_INFO << getHost() << ": Received the list of hosts";
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSE_EXIT,keto::server_common::Constants::RPC_COMMANDS::CLOSE_EXIT);
}

void RpcReceiveQueue::handleRegisterResponse(const std::string& command, const std::string& payload) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));

    this->accountHash = rpcPeerHelper.getAccountHash();
    this->setClientActive(rpcPeerHelper.isActive());

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::REGISTER_RPC_PEER_SERVER,rpcPeerHelper));

    // set the registered flag
    this->registered = true;

    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}


void RpcReceiveQueue::handleActivatePeer(const std::string& command, const std::string& payload) {
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload));
    keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);
    this->setClientActive(rpcPeerHelper.isActive());
    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ACTIVATE_RPC_PEER,rpcPeerHelper));
}

void RpcReceiveQueue::handleTransaction(const std::string& command, const std::string& payload) {
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));
    messageWrapperProtoHelper.setSessionHash(
            keto::server_common::VectorUtils().copyVectorToString(
                    keto::server_common::ServerInfo::getInstance()->getAccountHash()));

    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                            keto::server_common::Events::ROUTE_MESSAGE,messageWrapper)));


    std::string result = messageWrapperResponse.SerializeAsString();
    this->rpcSendQueuePtr->pushEntry(
            keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED,Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleBlock(const std::string& command, const std::string& payload) {

    // if this method is called the peer knows it is active and we can thus use it for further synchronization
    if (!clientIsActive()) {
        setClientActive(true);
    }
    blockTouch();
    keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
    signedBlockWrapperMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));
    KETO_LOG_INFO << "[RpcSession][" << getHost() << "][handleBlock] persist bock";
    keto::transaction_common::MessageWrapperResponseHelper messageWrapperResponseHelper(
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
                            keto::server_common::Events::BLOCK_PERSIST_MESSAGE,signedBlockWrapperMessage))));
    if (messageWrapperResponseHelper.isSuccess() || messageWrapperResponseHelper.getMsg().empty()) {
        std::string result = messageWrapperResponseHelper;
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED,
                                         Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
    } else {
        keto::proto::SignedBlockBatchRequest signedBlockBatchRequest;
        signedBlockBatchRequest.ParseFromString(messageWrapperResponseHelper.getBinaryMsg());
        KETO_LOG_INFO << "[RpcReceiveQueue::handleBlock] Chain is out of sync test serialization :" <<
            signedBlockBatchRequest.tangle_hashes_size();
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_REQUEST,
                                         messageWrapperResponseHelper.getMsg());

    }
}

void RpcReceiveQueue::consensusSessionResponse(const std::string& command, const std::string& sessionKey) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION,"OK");
}

void RpcReceiveQueue::consensusResponse(const std::string& command, const std::string& payload) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    keto::asn1::HashHelper hashHelper(payload,keto::common::StringEncoding::HEX);
    std::vector<uint8_t> result = buildConsensus(hashHelper);
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleRequestNetworkSessionKeysResponse(const std::string& command, const std::string& payload) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkSessionKeysResponse][" << getHost() << "] set the network session keys";
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(payload));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkSessionKeysResponse][" << getHost() << "] request the master network keys";
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS);
}

void RpcReceiveQueue::handleRequestNetworkMasterKeyResponse(const std::string& command, const std::string& payload) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkMasterKeyResponse][" << getHost() << "] Set the master keys";
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(payload));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkMasterKeyResponse][" << getHost() << "] After setting the master keys";
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS);
}

void RpcReceiveQueue::handleRequestNetworkKeysResponse(const std::string& command, const std::string& payload) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkKeysResponse][" << getHost() << "] Set the network keys";
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(payload));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_KEYS,networkKeysWrapper)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkKeysResponse][" << getHost() << "] After setting the network keys";
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES);
}

void RpcReceiveQueue::handleRequestNetworkFeesResponse(const std::string& command, const std::string& payload) {
    keto::transaction_common::FeeInfoMsgProtoHelper feeInfoMsgProtoHelper(
            Botan::hex_decode(payload));

    keto::proto::FeeInfoMsg feeInfoMsg = feeInfoMsgProtoHelper;
    feeInfoMsg = keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::FeeInfoMsg>(
                            keto::server_common::Events::NETWORK_FEE_INFO::SET_NETWORK_FEE,feeInfoMsg)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkFeesResponse][" << getHost() << "][" << this->getSessionId() << "] #######################################################";
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkFeesResponse][" << getHost() << "][" << this->getSessionId() << "] ######## Network intialization is now complete ########";
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkFeesResponse][" << getHost() << "][" << this->getSessionId() << "] #######################################################";


    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE,
                         keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE);
}

void RpcReceiveQueue::closeResponse(const std::string& command, const std::string& payload) {

    //KETO_LOG_INFO << "Client sending close to server [" << payload << "]";
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSE,keto::server_common::Constants::RPC_COMMANDS::CLOSE);
}

void RpcReceiveQueue::handleBlockSyncResponse(const std::string& command, const std::string& payload) {
    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));
    if (signedBlockBatchMessage.partial_result()) {
        setClientActive(false);
    }
    keto::transaction_common::MessageWrapperResponseHelper messageWrapperResponseHelper(
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                            keto::server_common::Events::BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage))));

    if (messageWrapperResponseHelper.isSuccess() || messageWrapperResponseHelper.getMsg().empty()) {
        std::string result = messageWrapperResponseHelper;
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_PROCESSED,
                                         Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
    } else {
        keto::proto::SignedBlockBatchRequest signedBlockBatchRequest;
        signedBlockBatchRequest.ParseFromString(messageWrapperResponseHelper.getBinaryMsg());
        KETO_LOG_INFO << "[RpcReceiveQueue::handleBlockSyncResponse] Chain is out of sync test serialization :" <<
        signedBlockBatchRequest.tangle_hashes_size();
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::MISSING_BLOCK_SYNC_REQUEST,
                                         messageWrapperResponseHelper.getMsg());

    }
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
    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                            keto::server_common::Events::MISSING_BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage)));

}

void RpcReceiveQueue::handleProtocolCheckRequest(const std::string& command, const std::string& payload) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->resetProtocolCheck();

    keto::asn1::HashHelper initHashHelper(payload,keto::common::StringEncoding::HEX);
    std::vector<uint8_t> result = buildConsensus(initHashHelper);
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleProtocolCheckAccept(const std::string& command, const std::string& payload) {

    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyProtocolCheck();

}

void RpcReceiveQueue::handleProtocolHeartbeat(const std::string& command, const std::string& payload) {
    keto::proto::ProtocolHeartbeatMessage protocolHeartbeatMessage;
    protocolHeartbeatMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(payload)));
    // clear out the election result cache every type a new election begins.
    electionResultCache.heartBeat(protocolHeartbeatMessage);
    keto::software_consensus::ConsensusSessionManager::getInstance()->initNetworkHeartbeat(protocolHeartbeatMessage);
}

void RpcReceiveQueue::handleElectionRequest(const std::string& command, const std::string& payload) {
    KETO_LOG_INFO << getHost() << "[handleElectionRequest]: handle the election";
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
    KETO_LOG_INFO << getHost() << "[handleElectionRequest]: return the election result";
}

void RpcReceiveQueue::handleElectionResponse(const std::string& command, const std::string& payload) {
    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::ElectionResultMessage>(
                    keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE,electionResultMessageProtoHelper));
}

void RpcReceiveQueue::handleElectionPublish(const std::string& command, const std::string& payload) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper)) {
        return;
    }

    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_PUBLISH).
        publish(electionPublishTangleAccountProtoHelper);
}

void RpcReceiveQueue::handleElectionConfirmation(const std::string& command, const std::string& payload) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        return;
    }

    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_CONFIRMATION).
        confirmation(electionConfirmationHelper);
}

void RpcReceiveQueue::handleRequestNetworkStatus(const std::string& command, const std::string& payload) {
    keto::proto::PublishedElectionInformation publishedElectionInformation;
    publishedElectionInformation =
            keto::server_common::fromEvent<keto::proto::PublishedElectionInformation>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
                            keto::server_common::Events::ROUTER_QUERY::GET_PUBLISHED_ELECTION_INFO,publishedElectionInformation)));

    std::string result = publishedElectionInformation.SerializeAsString();
    this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS,
                                     Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

void RpcReceiveQueue::handleResponseNetworkStatus(const std::string& command, const std::string& payload) {
    if (RpcClient::getInstance()->hasNetworkState()) {
        return;
    }
    keto::election_common::PublishedElectionInformationHelper publishedElectionInformationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(payload)));

    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
            keto::server_common::Events::ROUTER_QUERY::SET_PUBLISHED_ELECTION_INFO,publishedElectionInformationHelper));
    RpcClient::getInstance()->activateNetworkState();
}

void RpcReceiveQueue::handleInternalException(const std::string& command, const std::string& cause) {

    KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "] Processing failed for the command : " << command;
    if (command == keto::server_common::Constants::RPC_COMMANDS::ACCEPTED) {
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "][" << command << "] reconnect to the server";

        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "][" << command << "] force a reset of the session as it is currently invalid";
        keto::software_consensus::ConsensusSessionManager::getInstance()->resetSessionKey();

        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSED,
                                         keto::server_common::Constants::RPC_COMMANDS::CLOSED);


    } else if (command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS ||
    command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS  ||
    command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS ||
    command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) {

        // force the session
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "][" << command << "] reconnect to the server";
        if (cause == "Invalid Session exception."
        || cause == "Invalid password exception."
        || cause == "Out of date network slot."
        || cause == "The key data supplied is invalid."
        || cause.find("Index out of bounds") != std::string::npos) {
            KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "][" << command << "] force a reset of the session as it is curretly invalid";
            keto::software_consensus::ConsensusSessionManager::getInstance()->resetSessionKey();
        }

        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSED,
                                         keto::server_common::Constants::RPC_COMMANDS::CLOSED);

    } else if (command == keto::server_common::Constants::RPC_COMMANDS::HELLO ||
    command == keto::server_common::Constants::RPC_COMMANDS::GO_AWAY ||
    command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT ||
    command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS ||
    command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION ||
    command == keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS ||
    command == keto::server_common::Constants::RPC_COMMANDS::PEERS ||
    command == keto::server_common::Constants::RPC_COMMANDS::REGISTER ||
    command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST) {
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "] Attempt to reconnect";
        this->rpcSendQueuePtr->pushEntry(keto::server_common::Constants::RPC_COMMANDS::CLOSED,
                                         keto::server_common::Constants::RPC_COMMANDS::CLOSED);
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) {
        // this indicates the up stream server is currently out of sync and cannot be relied upon we therefore
        // need to use an alternative and mark this one as inactive until it is activated.
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << getHost() << "] Deactive this session and re-schedule the retry";
        if (this->clientIsActive()) {
            this->setClientActive(false);
        }
        // reschedule the block sync retry
        keto::proto::MessageWrapper messageWrapper;
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                keto::server_common::Events::BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY,messageWrapper));
    } else {
        KETO_LOG_INFO << "[RpcSession::handleInternalException] Ignore as no retry is required";
        KETO_LOG_INFO << this->getSessionId() << ": Setup connection for read : " << command << std::endl;
    }
}

std::vector<uint8_t> RpcReceiveQueue::buildConsensus(const keto::asn1::HashHelper& hashHelper) {
    keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper;
    moduleHashMessageHelper.setHash(hashHelper.operator keto::crypto::SecureVector());
    keto::proto::ModuleHashMessage moduleHashMessage = moduleHashMessageHelper.getModuleHashMessage();
    keto::proto::ConsensusMessage consensusMessage =
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ModuleHashMessage>(
                                    keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,moduleHashMessage)));
    keto::software_consensus::ConsensusSessionManager::getInstance()->setSession(consensusMessage);
    std::string result;
    consensusMessage.SerializePartialToString(&result);
    return keto::server_common::VectorUtils().copyStringToVector(result);
}


std::vector<uint8_t> RpcReceiveQueue::buildHeloMessage() {
    return keto::server_common::VectorUtils().copyStringToVector(keto::rpc_protocol::ServerHelloProtoHelper(this->keyLoaderPtr).setAccountHash(
            keto::server_common::ServerInfo::getInstance()->getAccountHash()).sign().operator std::string());
}

}
}
