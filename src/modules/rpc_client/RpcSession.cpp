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

#include "Route.pb.h"

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

#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"
#include "keto/rpc_protocol/PeerResponseHelper.hpp"

#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/transaction/Transaction.hpp"
#include "keto/server_common/TransactionHelper.hpp"

namespace keto {
namespace rpc_client {

// Report a failure
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

std::string RpcSession::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

RpcSession::RpcSession(
        std::shared_ptr<boost::asio::io_context> ioc, 
        std::shared_ptr<boostSsl::context> ctx, 
        bool peered,
        const std::string& host) :
        resolver(*ioc),
        ws_(*ioc, *ctx),
        peered(peered),
        host(host) {
    if (host.find(":") != std::string::npos) {
        std::vector<std::string> results;
        boost::split(results, host, [](char c){return c == ':';});
        this->host = results[0];
        this->port = results[1];
    } else {
        this->port = Constants::DEFAULT_PORT_NUMBER;
    }
    
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
        host.c_str(),
        port.c_str(),
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
        return fail(ec, "resolve");

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
    if(ec)
        return fail(ec, "connect");

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
        return fail(ec, "ssl_handshake");

    // Perform the websocket handshake
    ws_.async_handshake(host, "/",
        std::bind(
            &RpcSession::on_handshake,
            shared_from_this(),
            std::placeholders::_1));
}

void
RpcSession::on_handshake(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, "handshake");

    // Send the message
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::HELLO,buildHeloMessage());
    ws_.async_write(
        buffer_.data(),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}


void
RpcSession::on_write(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");
    
    buffer_.consume(buffer_.size());

    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        std::bind(
            &RpcSession::on_read,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void
RpcSession::on_read(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if(ec)
        return fail(ec, "read");
    
    std::lock_guard<std::mutex> guard(this->classMutex);
    
    std::stringstream ss;
    ss << boost::beast::buffers(buffer_.data());
    std::string data = ss.str();
    
    // Clear the buffer
    buffer_.consume(buffer_.size());
    
    keto::server_common::StringVector stringVector = keto::server_common::StringUtils(data).tokenize(" ");
    
    std::string command = stringVector[0];
    
    
    try {
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
        
        // Close the WebSocket connection
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
            helloConsensusResponse(command,stringVector[1],stringVector[2]);
            transactionPtr->commit();
            return;
        } if (command.compare(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY) == 0) {
            closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE,stringVector[1]);
            return;
        } if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED) == 0) {
            if (!peered) {
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
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
            
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
            
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION) == 0) {
            consensusSessionResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {
            consensusResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE) == 0) {

        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE_UPDATE) == 0) {

        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::SERVICES) == 0) {

        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
            closeResponse(command,stringVector[1]);
            transactionPtr->commit();
            return;
        }
        transactionPtr->commit();
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "Failed to process because : " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "Failed process the request : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "Failed to process : ";
    }
    
    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        std::bind(
            &RpcSession::on_read,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));

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
    
    if (this->peered && !this->accountHash.empty()) {
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
    
    ws_.async_close(websocket::close_code::normal,
            std::bind(
                &RpcSession::on_close,
                shared_from_this(),
                std::placeholders::_1));
}

void RpcSession::helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash) {
    keto::asn1::HashHelper initHashHelper(initHash,keto::common::StringEncoding::HEX);
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager().updateSessionKey(initVector);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,buildConsensus(initHashHelper));
    ws_.async_write(
        buffer_.data(),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void RpcSession::consensusSessionResponse(const std::string& command, const std::string& sessionKey) {
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager().updateSessionKey(initVector);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION,"OK");
    ws_.async_write(
        buffer_.data(),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void RpcSession::consensusResponse(const std::string& command, const std::string& message) {
    keto::asn1::HashHelper hashHelper(message,keto::common::StringEncoding::HEX);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS,buildConsensus(hashHelper));
    ws_.async_write(
        buffer_.data(),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void RpcSession::serverRequest(const std::string& command, const std::string& message) {
    
    boost::beast::ostream(buffer_) << 
            command << " " << message;
    ws_.async_write(
        buffer_.data(),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void RpcSession::peerResponse(const std::string& command, const std::string& message) {
    std::string response = keto::server_common::VectorUtils().copyVectorToString(
        Botan::hex_decode(message,true));
    keto::rpc_protocol::PeerResponseHelper peerResponseHelper(response);
    
    RpcSessionManager::getInstance()->setPeers(peerResponseHelper.getPeers());
    
    // Read a message into our buffer
    ws_.async_close(websocket::close_code::normal,
            std::bind(
                &RpcSession::on_close,
                shared_from_this(),
                std::placeholders::_1));
    
}

void RpcSession::handleRegisterRequest(const std::string& command, const std::string& message) {
    keto::proto::RpcPeer rpcPeer;
    rpcPeer = keto::server_common::fromEvent<keto::proto::RpcPeer>(
                keto::server_common::processEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                keto::server_common::Events::GET_NODE_ACCOUNT_ROUTING,rpcPeer)));
    std::string rpcValue;
    rpcPeer.SerializePartialToString(&rpcValue);
    std::vector<uint8_t> rpcAccountBytes =  keto::server_common::VectorUtils().copyStringToVector(
            rpcValue);
    
    boost::beast::ostream(buffer_) << 
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::REGISTER,
            rpcAccountBytes);
    ws_.async_write(
        buffer_.data(),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
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
    
    
}

void RpcSession::routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    std::string messageWrapperStr;
    std::cout << "Route transaction to a peer" << std::endl;
    messageWrapper.SerializeToString(&messageWrapperStr);
    boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::TRANSACTION
            << " " << Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true);
    
    ws_.text(ws_.got_text());
    ws_.async_write(
        buffer_.data(),
            std::bind(
                &RpcSession::on_outBoundWrite,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
}
    
void
RpcSession::on_outBoundWrite(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    std::cout << "Bytes have been written" << std::endl;
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");

    // Clear the buffer
    std::cout << "Consume the bytes from the buffer" << std::endl;
    buffer_.consume(buffer_.size());
    
}

}
}