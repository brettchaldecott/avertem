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


#include <boost/algorithm/string.hpp>

#include "keto/common/Log.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/ssl/RootCertificate.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/rpc_client/RpcSession.hpp"
#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/Exception.hpp"

#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"

#include "keto/software_consensus/ConsensusBuilder.hpp"

namespace keto {
namespace rpc_client {

// Report a failure
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

RpcSession::RpcSession(std::shared_ptr<boost::asio::io_context> ioc, 
    std::shared_ptr<boostSsl::context> ctx, const std::string& host) :
        resolver(*ioc),
        ws_(*ioc, *ctx),
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
    std::cout << "Send the consensus message" << std::endl;
    ws_.async_write(
        boost::asio::buffer(
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::HELLO,buildHeloMessage())),
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
    
    std::stringstream ss;
    ss << boost::beast::buffers(buffer_.data());
    std::string data = ss.str();
    std::cout << "The buffer is : " << data << std::endl;
    
    // Clear the buffer
    buffer_.consume(buffer_.size());
    
    keto::server_common::StringVector stringVector = keto::server_common::StringUtils(data).tokenize(" ");
    
    std::string command = stringVector[0];
    std::string payload;
    if (stringVector.size() == 2) {
        payload = stringVector[1];
    }
    
    
    // Close the WebSocket connection
    if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO) == 0) {
        
        
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
        
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
        
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {
        consensusResponse(command,payload);
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE) == 0) {
        
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE_UPDATE) == 0) {
        
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::SERVICES) == 0) {
        
    } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
        closeResponse(command,payload);
    }
    
    // Read a message into our buffer
    std::cout << "Read in more data" << std::endl;
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
    std::cout << "Close things off " << std::endl;
    // If we get here then the connection is closed gracefully

    // The buffers() function helps print a ConstBufferSequence
    std::stringstream ss;
    ss << boost::beast::buffers(buffer_.data());
    std::string data = ss.str();
    std::cout << "The buffer is : " << data << std::endl;
    
}

std::string RpcSession::buildHeloMessage() {
    return keto::rpc_protocol::ServerHelloProtoHelper(this->keyLoaderPtr).setAccountHash(
            keto::server_common::ServerInfo::getInstance()->getAccountHash()).sign().operator std::string();
}

std::string RpcSession::buildConsensus() {
    return keto::software_consensus::ConsensusBuilder(this->keyLoaderPtr).
            buildConsensus().getConsensus();
}

std::string RpcSession::buildMessage(const std::string& command, const std::string& message) {
    std::stringstream ss;
    ss << command << " " << message;
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

void RpcSession::consensusResponse(const std::string& command, const std::string& message) {
    ws_.async_write(
        boost::asio::buffer(
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS,buildConsensus())),
        std::bind(
            &RpcSession::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}



}
}