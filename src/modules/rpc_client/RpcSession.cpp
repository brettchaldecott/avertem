/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcSession.cpp
 * Author: ubuntu
 * 
 * Created on January 22, 2018, 12:32 PM
 */


#include <string>
#include <sstream>


#include <botan/hex.h>
#include <botan/base64.h>

#include <boost/beast/core.hpp>

#include <boost/algorithm/string.hpp>
#include <keto/rpc_protocol/NetworkKeysWrapperHelper.hpp>

#include "Route.pb.h"
#include "SoftwareConsensus.pb.h"
#include "HandShake.pb.h"
#include "BlockChain.pb.h"
#include "Protocol.pb.h"


#include "keto/common/Log.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/ssl/RootCertificate.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/rpc_client/RpcSession.hpp"
#include "keto/rpc_client/RpcSessionManager.hpp"
#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/Exception.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"
#include "keto/rpc_protocol/PeerResponseHelper.hpp"
#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"

#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/transaction/Transaction.hpp"
#include "keto/server_common/TransactionHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

namespace keto {
namespace rpc_client {

int sessionIndex = 0;

// Report a failure
void
RpcSession::fail(boost::system::error_code ec, const std::string& what)
{
    std::cerr << what << ": " << ec.message() << "\n";
    rpcPeer.incrementReconnectCount();
    if (what == Constants::SESSION::CONNECT) {
        std::cout << "Attempt to reconnect" << std::endl;
        RpcSessionManager::getInstance()->reconnect(rpcPeer);

        // force the close to be handled on this connection
        ws_.async_close(
                websocket::close_code::normal,
                boost::asio::bind_executor(
                        strand_,
                        std::bind(
                                &RpcSession::on_close,
                                shared_from_this(),
                                std::placeholders::_1)));
    }
}

// Report a failure
void RpcSession::processingFailed()
{
    rpcPeer.incrementReconnectCount();
    std::cout << "Attempt to reconnect" << std::endl;
    RpcSessionManager::getInstance()->reconnect(rpcPeer);

    // force the close to be handled on this connection
    ws_.async_close(
            websocket::close_code::normal,
            boost::asio::bind_executor(
                    strand_,
                    std::bind(
                            &RpcSession::on_close,
                            shared_from_this(),
                            std::placeholders::_1)));
}

std::string RpcSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSession::RpcSession(
        std::shared_ptr<boost::asio::io_context> ioc, 
        std::shared_ptr<boostSsl::context> ctx, 
        const RpcPeer& rpcPeer) :
        resolver(*ioc),
        ws_(*ioc, *ctx),
        strand_(ws_.get_executor()),
        rpcPeer(rpcPeer) {
    this->sessionNumber = sessionIndex++;
    std::cout << this->sessionNumber << ": [RpcSession] created a new session" << std::endl;
    // setup the key loader
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
    
}

RpcSession::~RpcSession() {
}

// Start the asynchronous operation
void
RpcSession::run()
{
    // Look up the domain name
    this->resolver.async_resolve(
        this->rpcPeer.getHost().c_str(),
        this->rpcPeer.getPort().c_str(),
        std::bind(
            &RpcSession::on_resolve,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void
RpcSession::on_resolve(
    boost::system::error_code ec,
    tcp::resolver::results_type results)
{
    if(ec)
        return fail(ec, Constants::SESSION::RESOLVE);

    // Make the connection on the IP address we get from a lookup
    boost::asio::async_connect(
        ws_.next_layer().next_layer(),
        results.begin(),
        results.end(),
        std::bind(
            &RpcSession::on_connect,
            shared_from_this(),
            std::placeholders::_1));
}

void
RpcSession::on_connect(boost::system::error_code ec)
{
    if(ec) {

        return fail(ec, Constants::SESSION::CONNECT);
    }

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        boostSsl::stream_base::client,
        std::bind(
            &RpcSession::on_ssl_handshake,
            shared_from_this(),
            std::placeholders::_1));
}

void
RpcSession::on_ssl_handshake(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, Constants::SESSION::SSL_HANDSHAKE);

    // Perform the websocket handshake
    ws_.async_handshake(
        rpcPeer.getHost(),
        "/",
        std::bind(
            &RpcSession::on_handshake,
            shared_from_this(),
            std::placeholders::_1));
}

void
RpcSession::on_handshake(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, Constants::SESSION::HANDSHAKE);

    // Send the message
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::HELLO,buildHeloMessage());
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}


void
RpcSession::on_write(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");

    std::cout << this->sessionNumber << ": [RpcSession][on_write] write the bytes and call read" << std::endl;
    buffer_.consume(buffer_.size());

    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void
RpcSession::on_read(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    std::cout << this->sessionNumber << ": [RpcSession][on_read] the on read method" << std::endl;
    boost::ignore_unused(bytes_transferred);
    if(ec)
        return fail(ec, "read");
        
    // parse the input
    std::cout << this->sessionNumber << ": [RpcSession][on_read] process the buffer" << std::endl;
    std::stringstream ss;
    ss << boost::beast::buffers(buffer_.data());
    std::string data = ss.str();
    std::cout << this->sessionNumber << ": [RpcSession][on_read] data : " << data << std::endl;
    keto::server_common::StringVector stringVector = keto::server_common::StringUtils(data).tokenize(" ");
    std::string command = data;
    if (stringVector.size() > 1) {
        command = stringVector[0];
    }

    // Clear the buffer
    std::cout << this->sessionNumber << ": [RpcSession][on_read] consume the buffer command is : " << command << std::endl;
    buffer_.consume(buffer_.size());
    std::cout << "Size of the buffer is : " << buffer_.size() << std::endl;

    try {
        std::cout << this->sessionNumber << ": [RpcSession][on_read] create a new transaction" << std::endl;
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
        
        // Close the WebSocket connection
        std::cout << this->sessionNumber << ": [RpcSession][on_read] process the command :" << command << std::endl;
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
            helloConsensusResponse(command,stringVector[1],stringVector[2]);
            transactionPtr->commit();
            return;
        } if (command.compare(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY) == 0) {
            closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE,stringVector[1]);
            return;
        } if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED) == 0) {
            if (!this->rpcPeer.getPeered()) {
                serverRequest(keto::server_common::Constants::RPC_COMMANDS::PEERS,
                        keto::server_common::Constants::RPC_COMMANDS::PEERS);
                transactionPtr->commit();
                return;
            } else {
                handleRegisterRequest(command, stringVector[1]);
                transactionPtr->commit();
                return;
            }
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
            peerResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REGISTER) == 0) {
            registerResponse(command, stringVector[1]);
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
            KETO_LOG_INFO << "[RpcSession] handle a transaction";
            handleTransaction(command,stringVector[1]);
            KETO_LOG_INFO << "[RpcSession] Transaction processed";
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
            KETO_LOG_INFO << "The transaction has been processed : " << stringVector[1];
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK) == 0) {
            KETO_LOG_INFO << "[RpcSession] handle a block";
            handleBlock(command,stringVector[1]);
            transactionPtr->commit();
            KETO_LOG_INFO << "[RpcSession] block processed";
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED) == 0) {
            KETO_LOG_INFO << "The transaction has been processed : " << stringVector[1];
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION) == 0) {
            consensusSessionResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {
            consensusResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS) == 0) {
            requestNetworkSessionKeysResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS) == 0) {
            requestNetworkMasterKeyResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS) == 0) {
            requestNetworkKeysResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) == 0) {
            requestNetworkFeesResponse(command,stringVector[1]);
            transactionPtr->commit();
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE) == 0) {

        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE_UPDATE) == 0) {

        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::SERVICES) == 0) {

        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
            closeResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_RETRY) == 0) {
            handleRetryResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) == 0) {
            handleBlockSyncResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST) == 0) {
            handleProtocolCheckRequest(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT) == 0) {
            handleProtocolCheckAccept(command,stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_HEARTBEAT) == 0) {
            handleProtocolHeartbeat(command,stringVector[1]);
        }


        transactionPtr->commit();
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[RPCSession][" << this->sessionNumber << "]" << command << " : Failed to process because : " << boost::diagnostic_information(ex,true);
        return processingFailed();
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[RPCSession][" << this->sessionNumber << "]" << command << " : Failed to process because : " << boost::diagnostic_information(ex,true);
        return processingFailed();
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[RPCSession][" << this->sessionNumber << "]" << command << " : Failed process the request : " << ex.what();
        return processingFailed();
    } catch (...) {
        KETO_LOG_ERROR << "[RPCSession][" << this->sessionNumber << "]" << command << " : Failed process the request" << std::endl;
        return processingFailed();
    }
    
    // Read a message into our buffer
    std::cout << this->sessionNumber << ": Handle the read response : " << command << std::endl;
    ws_.async_read(
        buffer_,
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));

}

void
RpcSession::on_close(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, "close");
    // If we get here then the connection is closed gracefully

    // The buffers() function helps print a ConstBufferSequence
    std::stringstream ss;
    ss << boost::beast::buffers(buffer_.data());
    std::string data = ss.str();
    
    if (this->rpcPeer.getPeered() && !this->accountHash.empty()) {
        RpcSessionManager::getInstance()->removeAccountSessionMapping(this->accountHash);
    }
}

std::vector<uint8_t> RpcSession::buildHeloMessage() {
    return keto::server_common::VectorUtils().copyStringToVector(keto::rpc_protocol::ServerHelloProtoHelper(this->keyLoaderPtr).setAccountHash(
            keto::server_common::ServerInfo::getInstance()->getAccountHash()).sign().operator std::string());
}

std::vector<uint8_t> RpcSession::buildConsensus(const keto::asn1::HashHelper& hashHelper) {
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

std::string RpcSession::buildMessage(const std::string& command, const std::string& message) {
    std::stringstream ss;
    ss << command << " " << message;
    return ss.str();
}

std::string RpcSession::buildMessage(const std::string& command, const std::vector<uint8_t>& message) {
    std::stringstream ss;
    ss << command << " " << Botan::hex_encode(message);
    return ss.str();
}

void RpcSession::closeResponse(const std::string& command, const std::string& message) {
    
    KETO_LOG_ERROR << "Server closing connection because [" << message << "]";
    
    ws_.async_close(
        websocket::close_code::normal,
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_close,
                shared_from_this(),
                std::placeholders::_1)));
}

void RpcSession::helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash) {
    keto::asn1::HashHelper initHashHelper(initHash,keto::common::StringEncoding::HEX);
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,buildConsensus(initHashHelper));
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void RpcSession::consensusSessionResponse(const std::string& command, const std::string& sessionKey) {
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION,"OK");
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void RpcSession::consensusResponse(const std::string& command, const std::string& message) {
    keto::asn1::HashHelper hashHelper(message,keto::common::StringEncoding::HEX);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS,buildConsensus(hashHelper));
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void RpcSession::serverRequest(const std::string& command, const std::string& message) {
    
    boost::beast::ostream(buffer_) <<
        buildMessage(command,message);

    std::cout << this->sessionNumber << ": Send the server request : " << command << std::endl;
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
    std::cout << this->sessionNumber << ": Sent the server request : " << command << std::endl;
}

void RpcSession::peerResponse(const std::string& command, const std::string& message) {
    std::string response = keto::server_common::VectorUtils().copyVectorToString(
        Botan::hex_decode(message,true));
    keto::rpc_protocol::PeerResponseHelper peerResponseHelper(response);
    
    RpcSessionManager::getInstance()->setPeers(peerResponseHelper.getPeers());
    
    // Read a message into our buffer
    ws_.async_close(
        websocket::close_code::normal,
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_close,
                shared_from_this(),
                std::placeholders::_1)));
    
}

void RpcSession::handleRegisterRequest(const std::string& command, const std::string& message) {

    // notify the accepted
    keto::router_utils::RpcPeerHelper rpcPeerHelper;
    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    keto::proto::RpcPeer rpcPeer = rpcPeerHelper;
    std::string rpcValue;
    rpcPeer.SerializePartialToString(&rpcValue);
    std::vector<uint8_t> rpcAccountBytes =  keto::server_common::VectorUtils().copyStringToVector(
            rpcValue);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::REGISTER,
            rpcAccountBytes);
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void RpcSession::handleTransaction(const std::string& command, const std::string& message) {
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
        keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));
    messageWrapperProtoHelper.setSessionHash(
            keto::server_common::VectorUtils().copyVectorToString(    
                keto::server_common::ServerInfo::getInstance()->getAccountHash()));

    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    keto::proto::MessageWrapperResponse messageWrapperResponse = 
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
            keto::server_common::Events::ROUTE_MESSAGE,messageWrapper)));

    std::string result = messageWrapperResponse.SerializeAsString();
    boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED
            << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
    
    ws_.async_write(
        buffer_.data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void RpcSession::handleBlock(const std::string& command, const std::string& message) {
    keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
    signedBlockWrapperMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
                            keto::server_common::Events::BLOCK_PERSIST_MESSAGE,signedBlockWrapperMessage)));

    std::string result = messageWrapperResponse.SerializeAsString();
    boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED
                                   << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);

    ws_.async_write(
            buffer_.data(),
            boost::asio::bind_executor(
                    strand_,
                    std::bind(
                            &RpcSession::on_write,
                            shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2)));
}


void RpcSession::handleBlockSyncResponse(const std::string& command, const std::string& message) {
    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                            keto::server_common::Events::BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage)));

    std::string result = messageWrapperResponse.SerializeAsString();
    boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_PROCESSED
                                   << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);

    ws_.async_write(
            buffer_.data(),
            boost::asio::bind_executor(
                    strand_,
                    std::bind(
                            &RpcSession::on_write,
                            shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2)));
}

void RpcSession::handleProtocolCheckRequest(const std::string& command, const std::string& message) {

    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->resetProtocolCheck();

    keto::asn1::HashHelper initHashHelper(message,keto::common::StringEncoding::HEX);

    boost::beast::ostream(buffer_) <<
                                   buildMessage(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE,buildConsensus(initHashHelper));
    ws_.async_write(
            buffer_.data(),
            boost::asio::bind_executor(
                    strand_,
                    std::bind(
                            &RpcSession::on_write,
                            shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2)));
}


void RpcSession::handleProtocolCheckAccept(const std::string& command, const std::string& message) {

    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyProtocolCheck();

}

void RpcSession::handleProtocolHeartbeat(const std::string& command, const std::string& message) {
    keto::proto::ProtocolHeartbeatMessage protocolHeartbeatMessage;
    protocolHeartbeatMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    keto::software_consensus::ConsensusSessionManager::getInstance()->initNetworkHeartbeat(protocolHeartbeatMessage);
}


void RpcSession::registerResponse(const std::string& command, const std::string& message) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper;
    rpcPeerHelper.setAccountHash(Botan::hex_decode(message));
    
    this->accountHash = rpcPeerHelper.getAccountHash();
    RpcSessionManager::getInstance()->setAccountSessionMapping(rpcPeerHelper.getAccountHashString(),
            shared_from_this());
    
    keto::proto::RpcPeer rpcPeer = (keto::proto::RpcPeer)rpcPeerHelper;
    rpcPeer = keto::server_common::fromEvent<keto::proto::RpcPeer>(
                keto::server_common::processEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                keto::server_common::Events::REGISTER_RPC_PEER,rpcPeer)));

    serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}

void RpcSession::requestNetworkSessionKeysResponse(const std::string& command, const std::string& message) {
    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyAccepted();

    std::cout << this->sessionNumber << ": Process the hex message" << std::endl;
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    std::cout << this->sessionNumber << ": Set the network session keys" << std::endl;
    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

    std::cout << this->sessionNumber << ": Set request the master network keys" << std::endl;
    serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS);
}


void RpcSession::requestNetworkMasterKeyResponse(const std::string& command, const std::string& message) {
    std::cout << this->sessionNumber << ": Process the master network key" << std::endl;
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

    std::cout << this->sessionNumber << ": Request the network keys" << std::endl;
    serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS);
}

void RpcSession::requestNetworkKeysResponse(const std::string& command, const std::string& message) {
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_KEYS,networkKeysWrapper)));

    serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES);
}

void RpcSession::requestNetworkFeesResponse(const std::string& command, const std::string& message) {
    keto::transaction_common::FeeInfoMsgProtoHelper feeInfoMsgProtoHelper(
            Botan::hex_decode(message));

    keto::proto::FeeInfoMsg feeInfoMsg = feeInfoMsgProtoHelper;
    feeInfoMsg = keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::FeeInfoMsg>(
                            keto::server_common::Events::NETWORK_FEE_INFO::SET_NETWORK_FEE,feeInfoMsg)));

}

void RpcSession::handleRetryResponse(const std::string& command, const std::string& message) {
    if (message == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS ||
        message == keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS  ||
        message == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS ||
        message == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) {

        std::cout << "Process the retry response : " << message << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));

        std::cout << "Send the retry : " << message << std::endl;
        serverRequest(message,
                      message);
        std::cout << "After sending the retry : " << message << std::endl;
    } else {
        std::cout << "Attempt to reconnect" << std::endl;
        RpcSessionManager::getInstance()->reconnect(rpcPeer);

        // Read a message into our buffer
        ws_.async_close(
                websocket::close_code::normal,
                boost::asio::bind_executor(
                        strand_,
                        std::bind(
                                &RpcSession::on_close,
                                shared_from_this(),
                                std::placeholders::_1)));
    }
}

void RpcSession::routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
    std::string messageWrapperStr = messageWrapper.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);
    
    MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
    boost::beast::ostream(*multiBufferPtr) << buildMessage(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
            messageBytes);
    
    ws_.async_write(
        multiBufferPtr->data(),
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &RpcSession::on_outBoundWrite,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2,
                multiBufferPtr)));
}


void RpcSession::requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    std::string messageWrapperStr = signedBlockBatchRequest.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
    boost::beast::ostream(*multiBufferPtr) << buildMessage(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
            messageBytes);

    ws_.async_write(
            multiBufferPtr->data(),
            boost::asio::bind_executor(
            strand_,
            std::bind(
                    &RpcSession::on_outBoundWrite,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2,
                    multiBufferPtr)));
}


void RpcSession::pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
    std::string messageWrapperStr = signedBlockWrapperMessage.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
    boost::beast::ostream(*multiBufferPtr) << buildMessage(keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                                                           messageBytes);

    ws_.async_write(
            multiBufferPtr->data(),
            boost::asio::bind_executor(
                    strand_,
                    std::bind(
                            &RpcSession::on_outBoundWrite,
                            shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2,
                            multiBufferPtr)));
}

    
void
RpcSession::on_outBoundWrite(
    boost::system::error_code ec,
    std::size_t bytes_transferred,
    MultiBufferPtr multiBufferPtr)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");

    // Clear the buffer
    multiBufferPtr->consume(multiBufferPtr->size());
}

RpcPeer RpcSession::getPeer() {
    return this->rpcPeer;
}

}
}