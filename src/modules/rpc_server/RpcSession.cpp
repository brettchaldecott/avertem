//
// Created by Brett Chaldecott on 2021/07/19.
//

#include <botan/hex.h>
#include <botan/base64.h>
#include <google/protobuf/message_lite.h>

#include "keto/rpc_server/RpcSession.hpp"
#include "keto/rpc_server/RpcSessionManager.hpp"
#include "keto/rpc_server/Exception.hpp"
#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/VectorUtils.hpp"


#include "keto/server_common/Constants.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/rpc_server/RpcServer.hpp"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

std::string RpcSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSession::RpcSession(int sessionId,boost::asio::ip::tcp::socket&& socket, sslBeast::context& ctx) : sessionId(sessionId) {
    this->rpcSessionSocketPtr = RpcSessionSocketPtr(new RpcSessionSocket(sessionId,std::move(socket), ctx));
    this->rpcServerProtocolPtr = RpcServerProtocolPtr(new RpcServerProtocol(sessionId));
}

RpcSession::~RpcSession() {

}

void RpcSession::start() {
    this->rpcSessionSocketPtr->start(rpcServerProtocolPtr);
}

void RpcSession::stop() {
    this->rpcSessionSocketPtr->stop();
}

int RpcSession::getSessionId() {
    return this->sessionId;
}

std::string RpcSession::getAccount() {
    return this->rpcServerProtocolPtr->getReceiveQueue()->getAccountHashStr();
}

std::string RpcSession::getAccountHash() {
    return this->rpcServerProtocolPtr->getReceiveQueue()->getAccountHash();
}

void RpcSession::join() {
    this->rpcSessionSocketPtr->join();
    this->rpcServerProtocolPtr->join();
}

bool RpcSession::isActive() {
    return this->rpcServerProtocolPtr->getReceiveQueue()->clientIsActive();
}

bool RpcSession::isRegistered() {
    return this->rpcServerProtocolPtr->getReceiveQueue()->isRegistered();
}

time_t RpcSession::getLastBlockTouch() {
    return this->rpcServerProtocolPtr->getReceiveQueue()->getLastBlockTouch();
}


void RpcSession::routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
    if (!isRegistered()) {
        return;
    }
    std::string messageWrapperStr;
    messageWrapper.SerializeToString(&messageWrapperStr);
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
                  Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
}


void RpcSession::pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
    if (!isRegistered()) {
        return;
    }
    KETO_LOG_INFO << "[RpcServer::pushBlock] " << getAccount() << " push block to server";
    std::string messageWrapperStr;
    signedBlockWrapperMessage.SerializeToString(&messageWrapperStr);
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                                                          Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));

}

void RpcSession::performNetworkSessionReset() {
    KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]Attempt to perform the protocol reset via forcing the client to say hello";
    if (!isRegistered()) {
        return;
    }
    std::stringstream ss;
    ss << Botan::hex_encode(RpcServer::getInstance()->getSecret())
       << " " << Botan::hex_encode(this->rpcServerProtocolPtr->getReceiveQueue()->generateSession());
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,
                                                          ss.str());
    KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]After requesting the protocol reset from peers";
}


void RpcSession::performProtocolCheck() {
    KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]Attempt to perform the protocol check";
    if (!isRegistered()) {
        return;
    }
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST,
                                                          Botan::hex_encode(this->rpcServerProtocolPtr->getReceiveQueue()->generateSession()));
    KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]After requesting the protocol check from peers";
}

void RpcSession::performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage) {
    //KETO_LOG_DEBUG << "Attempt to perform the protocol heartbeat";
    if (!isRegistered()) {
        return;
    }
    this->rpcServerProtocolPtr->getReceiveQueue()->performNetworkHeartbeat(protocolHeartbeatMessage);
    std::string messageWrapperStr;
    protocolHeartbeatMessage.SerializeToString(&messageWrapperStr);
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_HEARTBEAT,
                                                          Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
    //KETO_LOG_DEBUG << "After requesting the protocol heartbeat";
}

bool RpcSession::electBlockProducer() {
    KETO_LOG_INFO << getAccount() << "[electBlockProducer]: elect block producer";
    if (!this->isActive()) {
        return false;
    }
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper;
    electionPeerMessageProtoHelper.setAccount(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPeerMessageProtoHelper);
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
    KETO_LOG_INFO << getAccount() << "[electBlockProducer]: after invoking election.";
    return true;
}

void RpcSession::activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Activate this node with its peer";
    if (!isRegistered()) {
        return;
    }
    std::string rpcValue = rpcPeerHelper;
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE,
                                                          Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true));
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Activate this node with its peer";
}

void RpcSession::requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    if (!isActive()) {
        return;
    }
    std::string messageWrapperStr = signedBlockBatchRequest.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);
    KETO_LOG_INFO << "[session::requestBlockSync][" << getAccount() << "] request the block sync from a";
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
    //KETO_LOG_DEBUG << "[session::requestBlockSync][" << getAccount() << "] The request for [" <<
    //              keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST << "]";
}

void RpcSession::electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerPublish]: publish the election result";
    // prevent echo propergation at the boundary
    if (!isRegistered()) {
        return;
    }
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPublishTangleAccountProtoHelper);
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerPublish]: published the election result";
}

void RpcSession::electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerConfirmation]: confirmation of the election results";
    // p1revent echo propergation at the boundary
    if (!isRegistered()) {
        return;
    }
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionConfirmationHelper);
    this->rpcServerProtocolPtr->getSendQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                                                          Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerConfirmation]: confirmation sent for election results";
}

}
}
