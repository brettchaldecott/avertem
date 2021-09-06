//
// Created by Brett Chaldecott on 2021/08/11.
//

#include "keto/rpc_client/RpcClientSession.hpp"

#include <botan/hex.h>

#include "keto/rpc_client/RpcReceiveQueue.hpp"
#include "keto/rpc_client/RpcSendQueue.hpp"
#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/RpcClient.hpp"

#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/TransactionHelper.hpp"

#include "keto/transaction/Transaction.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"
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


std::string RpcClientSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcClientSession::RpcClientSession(int sessionId,
                 std::shared_ptr<net::io_context>& ioc,
                 std::shared_ptr<sslBeast::context>& ctx,
                 const RpcPeer& rpcPeer) : sessionId(sessionId), rpcPeer(rpcPeer) {
    this->rpcSessionSocketPtr = RpcSessionSocketPtr(new RpcSessionSocket(sessionId,ioc,ctx,rpcPeer));
    this->rpcClientProtocolPtr = RpcClientProtocolPtr(new RpcClientProtocol(sessionId,rpcPeer));
}

RpcClientSession::~RpcClientSession() {

}

void RpcClientSession::start() {
    this->rpcSessionSocketPtr->start(rpcClientProtocolPtr);
}

void RpcClientSession::stop() {
    this->rpcSessionSocketPtr->stop();
}

int RpcClientSession::getSessionId() {
    return this->sessionId;
}

std::string RpcClientSession::getAccount() {
    return this->rpcClientProtocolPtr->getReceiveQueue()->getAccountHashStr();
}

std::string RpcClientSession::getAccountHash() {
    return this->rpcClientProtocolPtr->getReceiveQueue()->getAccountHash();
}

std::string RpcClientSession::getHost() {
    return this->rpcPeer.getHost();
}

void RpcClientSession::join() {
    KETO_LOG_INFO << "[RpcClientSession::join] Rpc session join";
    this->rpcSessionSocketPtr->join();
    KETO_LOG_INFO << "[RpcClientSession::join] Protocol join";
    this->rpcClientProtocolPtr->join();
    KETO_LOG_INFO << "[RpcClientSession::join] after joins";
}

bool RpcClientSession::isRegistered() {
    return this->rpcClientProtocolPtr->getReceiveQueue()->isRegistered();
}

bool RpcClientSession::isActive() {
    return this->rpcClientProtocolPtr->getReceiveQueue()->clientIsActive();
}

bool RpcClientSession::isReconnect() {
    return this->rpcSessionSocketPtr->isReconnect();
}

RpcPeer RpcClientSession::getRpcPeer() {
    return this->rpcPeer;
}

time_t RpcClientSession::getLastBlockTouch() {
    return this->rpcClientProtocolPtr->getReceiveQueue()->getLastBlockTouch();
}

void RpcClientSession::routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
    if (!isRegistered()) {
        return;
    }
    std::string messageWrapperStr = messageWrapper.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
                                                          Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
}

void RpcClientSession::activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    if (!isRegistered()) {
        return;
    }
    std::string rpcValue = rpcPeerHelper;
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE,
                                                          Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true));
}

void RpcClientSession::requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    if (!isRegistered()) {
        return;
    }
    std::string messageWrapperStr = signedBlockBatchRequest.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    KETO_LOG_INFO << "[RpcSession::requestBlockSync] Requesting block sync from [" << getRpcPeer().getPeer() << "]";
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));

}

void RpcClientSession::pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
    if (!isRegistered()) {
        return;
    }

    KETO_LOG_INFO << "[RpcSession::pushBlock][" << getRpcPeer().getPeer() << "][" << this->getSessionId() << "] handle the push request";
    std::string messageWrapperStr;
    signedBlockWrapperMessage.SerializeToString(&messageWrapperStr);
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                                                          Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
}

void RpcClientSession::electBlockProducer() {
    if (!isRegistered()) {
        return;
    }
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper;
    electionPeerMessageProtoHelper.setAccount(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPeerMessageProtoHelper);
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
}


void RpcClientSession::electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    if (!isRegistered()) {
        return;
    }
    if (this->rpcClientProtocolPtr->getReceiveQueue()->containsPublishAccount(electionPublishTangleAccountProtoHelper.getAccount())) {
        return;
    }

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPublishTangleAccountProtoHelper);
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
}


void RpcClientSession::electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
    // prevent echo propergation at the boundary
    if (!isRegistered()) {
        return;
    }
    if (this->rpcClientProtocolPtr->getReceiveQueue()->containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        return;
    }

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionConfirmationHelper);
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
}


void RpcClientSession::pushRpcPeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    if (!isRegistered()) {
        return;
    }
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            rpcPeerHelper);
    this->rpcClientProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
}

}
}