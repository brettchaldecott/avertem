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
#include <mutex>
#include <chrono>

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

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_client {

int sessionIndex = 0;

RpcSession::BufferCache::BufferCache() {

}

RpcSession::BufferCache::~BufferCache() {
    for (boost::beast::flat_buffer* buffer : buffers) {
        delete buffer;
    }
    buffers.clear();
}

boost::beast::flat_buffer* RpcSession::BufferCache::create() {
    boost::beast::flat_buffer* buffer = new boost::beast::flat_buffer();
    this->buffers.insert(buffer);
    return buffer;
}

void RpcSession::BufferCache::remove(boost::beast::flat_buffer* buffer) {
    this->buffers.erase(buffer);
    delete buffer;
}

RpcSession::BufferScope::BufferScope(const BufferCachePtr& bufferCachePtr,
        boost::beast::flat_buffer* buffer) : bufferCachePtr(bufferCachePtr), buffer(buffer) {
}

RpcSession::BufferScope::~BufferScope() {
    bufferCachePtr->remove(buffer);
}

RpcSession::ReadQueue::ReadQueue(const RpcSessionPtr& session) : session(session) {
}

RpcSession::ReadQueue::~ReadQueue() {
    deactivate();
}

bool RpcSession::ReadQueue::isActive() {
    std::unique_lock<std::mutex> guard(classMutex);
    return this->active;
}

void RpcSession::ReadQueue::activate() {
    std::unique_lock<std::mutex> guard(classMutex);
    queueThreadPtr = std::shared_ptr<std::thread>(new std::thread(
            [this]
            {
                this->run();
            }));
    this->active = true;
}

void RpcSession::ReadQueue::deactivate() {
    {
        std::unique_lock<std::mutex> guard(classMutex);
        if (!this->active) {
            if (this->session) {
                this->session.reset();
            }
            return;
        }
        this->active = false;
        stateCondition.notify_all();
    }
    if (queueThreadPtr) {
        queueThreadPtr->join();
        queueThreadPtr.reset();
    }
    if (this->session) {
        this->session.reset();
    }
}

void RpcSession::ReadQueue::pushEntry(const std::string& command, const keto::server_common::StringVector& stringVector) {
    std::unique_lock<std::mutex> guard(classMutex);
    this->readQueue.push_back(ReadQueueEntryPtr(new ReadQueueEntry(command,stringVector)));
    stateCondition.notify_all();
}

RpcSession::ReadQueueEntryPtr RpcSession::ReadQueue::popEntry() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    if (!this->readQueue.empty()) {
        ReadQueueEntryPtr result = this->readQueue.front();
        this->readQueue.pop_front();
        return result;
    }
    this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
            Constants::DEFAULT_RPC_CLIENT_QUEUE_DELAY));
    return RpcSession::ReadQueueEntryPtr();
}

void RpcSession::ReadQueue::run() {
    while(this->isActive() && !this->session->isClosed()) {
        RpcSession::ReadQueueEntryPtr entry = popEntry();
        if (entry) {
            this->session->processQueueEntry(entry);
        }
    }
}

// Report a failure
void
RpcSession::fail(boost::system::error_code ec, const std::string& what) {
    KETO_LOG_ERROR << "Failed processing : " << what << ": " << ec.message();
    rpcPeer.incrementReconnectCount();
    if (what == Constants::SESSION::CONNECT) {
        KETO_LOG_DEBUG << this->sessionNumber << ":Attempt to reconnect";
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
    }

    if (what != "close" && what != Constants::SESSION::RESOLVE && what != Constants::SESSION::SSL_HANDSHAKE
                                                                  && what != Constants::SESSION::HANDSHAKE) {
        ws_.async_close(websocket::close_code::normal,
                        beast::bind_front_handler(
                                &RpcSession::on_close,
                                shared_from_this()));
    }

    if (this->readQueuePtr) {
        this->readQueuePtr->deactivate();
        this->readQueuePtr.reset();
    }


}

std::string RpcSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSession::RpcSession(
        std::shared_ptr<net::io_context> ioc,
        std::shared_ptr<sslBeast::context> ctx,
        const RpcPeer& rpcPeer) :
        reading(false),
        closed(false),
        active(false),
        registered(false),
        resolver(net::make_strand(*ioc)),
        ws_(net::make_strand(*ioc), *ctx),
        rpcPeer(rpcPeer) {
    this->sessionNumber = sessionIndex++;
    ws_.auto_fragment(false);
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession] created a new session";
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
    if (this->readQueuePtr) {
        this->readQueuePtr->deactivate();
        this->readQueuePtr.reset();
    }
}

// Start the asynchronous operation
void
RpcSession::run()
{

    // Look up the domain name
    this->resolver.async_resolve(
            this->rpcPeer.getHost().c_str(),
            this->rpcPeer.getPort().c_str(),
            beast::bind_front_handler(
                    &RpcSession::on_resolve,
                    shared_from_this()));
}

void
RpcSession::on_resolve(
    boost::system::error_code ec,
    tcp::resolver::results_type results)
{
    if(ec)
        return fail(ec, Constants::SESSION::RESOLVE);

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                    &RpcSession::on_connect,
                    shared_from_this()));
}

void
RpcSession::on_connect(boost::system::error_code ec,tcp::resolver::results_type::endpoint_type)
{
    if(ec) {

        return fail(ec, Constants::SESSION::CONNECT);
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
            sslBeast::stream_base::client,
            beast::bind_front_handler(
                    &RpcSession::on_ssl_handshake,
                    shared_from_this()));
}

void
RpcSession::on_ssl_handshake(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, Constants::SESSION::SSL_HANDSHAKE);

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(
            websocket::stream_base::timeout::suggested(
                    beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

    // Perform the websocket handshake
    ws_.async_handshake(rpcPeer.getHost(), "/",
                        beast::bind_front_handler(
                                &RpcSession::on_handshake,
                                shared_from_this()));
}

void
RpcSession::on_handshake(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, Constants::SESSION::HANDSHAKE);

    this->readQueuePtr = ReadQueuePtr(new RpcSession::ReadQueue(shared_from_this()));
    this->readQueuePtr->activate();


    // Send the message
    std::stringstream ss;
    ss <<
            buildMessage(keto::server_common::Constants::RPC_COMMANDS::HELLO,buildHeloMessage());
    send(ss.str());

    // do the read
    do_read();
}


void
RpcSession::on_write(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_write] handle the complete write call";
    boost::ignore_unused(bytes_transferred);
    queue_.pop_front();

    if(ec)
        return fail(ec, "write");

    if (queue_.size()) {
        sendFirstQueueMessage();
    }
    //if (!reading) {
    //    reading=true;
    //    do_read();
    //}
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_write] handled the completed write call";

}

void
RpcSession::do_read() {
    // Read a message into our buffer
    ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                    &RpcSession::on_read,
                    shared_from_this()));

}

void
RpcSession::on_read(
    boost::system::error_code ec,
    std::size_t bytes_transferred) {
    keto::server_common::StringVector stringVector;
    std::string command;

    if (ec)
        return fail(ec, "read");

    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_read] the on read method";
    boost::ignore_unused(bytes_transferred);

    // parse the input
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_read] process the buffer";
    std::stringstream ss;
    ss << boost::beast::make_printable(buffer_.data());
    std::string data = ss.str();
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_read] data : " << data;
    stringVector = keto::server_common::StringUtils(data).tokenize(" ");
    command = data;
    if (stringVector.size() > 1) {
        command = stringVector[0];
    }
    // Clear the buffer
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_read] consume the buffer command is : " << command;
    buffer_.consume(buffer_.size());
    KETO_LOG_DEBUG << "Size of the buffer is : " << buffer_.size();

    this->readQueuePtr->pushEntry(command,stringVector);

    if (this->isClosed()) {
        KETO_LOG_INFO << this->sessionNumber << " Closing the session";
        do_close();
    } else {
        do_read();
    }
}


void RpcSession::processQueueEntry(const ReadQueueEntryPtr& readQueueEntryPtr) {

    std::string command = readQueueEntryPtr->getCommand();
    keto::server_common::StringVector stringVector = readQueueEntryPtr->getStringVector();
    std::string message;
    try {
        KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_read] create a new transaction";
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();

        // Close the WebSocket connection
        KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][on_read] process the command :" << command;
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
            message = helloConsensusResponse(command, stringVector[1], stringVector[2]);
        }
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY) == 0) {
            closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE, stringVector[1]);
        }
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED) == 0) {
            if (!this->rpcPeer.getPeered()) {
                message = serverRequest(keto::server_common::Constants::RPC_COMMANDS::PEERS,
                                        keto::server_common::Constants::RPC_COMMANDS::PEERS);
            } else if (this->isRegistered()) {
                message = handleRequestNetworkSessionKeys(command, stringVector[1]);
            } else {
                message = handleRegisterRequest(command, stringVector[1]);
            }
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
            handlePeerResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REGISTER) == 0) {
            message = handleRegisterResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE) == 0) {
            KETO_LOG_DEBUG << "[RpcSession] handle the activate";
            handleActivatePeer(command, stringVector[1]);
            KETO_LOG_DEBUG << "[RpcSession] handle the activate";
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
            KETO_LOG_DEBUG << "[RpcSession] handle a block";
            message = handleTransaction(command, stringVector[1]);
            KETO_LOG_DEBUG << "[RpcSession] Transaction processed";
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
            KETO_LOG_DEBUG << "The transaction has been processed : " << stringVector[1];
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK) == 0) {
            KETO_LOG_DEBUG << "[RpcSession] handle a block";
            message = handleBlock(command, stringVector[1]);
            KETO_LOG_DEBUG << "[RpcSession] block processed";
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED) == 0) {
            KETO_LOG_DEBUG << "The block has been processed : " << stringVector[1];
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION) == 0) {
            message = consensusSessionResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {
            message = consensusResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS) ==
                   0) {
            message = handleRequestNetworkSessionKeysResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS) ==
                   0) {
            message = handleRequestNetworkMasterKeyResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS) == 0) {
            message = handleRequestNetworkKeysResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) == 0) {
            message = handleRequestNetworkFeesResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
            closeResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_RETRY) == 0) {
            message = handleRetryResponse(stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) == 0) {
            message = handleBlockSyncRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                                             stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) == 0) {
            message = handleBlockSyncResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST) == 0) {
            message = handleProtocolCheckRequest(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT) == 0) {
            handleProtocolCheckAccept(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_HEARTBEAT) == 0) {
            handleProtocolHeartbeat(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST) == 0) {
            message = handleElectionRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                                            stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE) == 0) {
            handleElectionResponse(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE,
                                   stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH) == 0) {
            handleElectionPublish(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                                  stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION) == 0) {
            handleElectionConfirmation(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                                       stringVector[1]);
        }
        transactionPtr->commit();
    } catch (keto::common::Exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed to process because : " << boost::diagnostic_information_what(ex, true);
        message = handleRetryResponse(command);
    } catch (boost::exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed to process because : " << boost::diagnostic_information_what(ex, true);
        message = handleRetryResponse(command);
    } catch (std::exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed process the request : " << ex.what();
        message = handleRetryResponse(command);
    } catch (...) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed process the request" << std::endl;
        message = handleRetryResponse(command);
    }

    if (!message.empty() && !this->isClosed()) {
        send(message);
    }

    // Read a message into our buffer
    KETO_LOG_DEBUG << this->sessionNumber << ": Finished the command : " << command;
}


void
RpcSession::do_close() {
    // Close the WebSocket connection
    ws_.async_close(websocket::close_code::normal,
                    beast::bind_front_handler(
                            &RpcSession::on_close,
                            shared_from_this()));
}

void
RpcSession::on_close(boost::system::error_code ec)
{
    if (this->rpcPeer.getPeered()) {
        keto::router_utils::RpcPeerHelper rpcPeerHelper;
        rpcPeerHelper.setAccountHash(this->accountHash);
        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                        keto::server_common::Events::DEREGISTER_RPC_PEER,rpcPeerHelper));
    }

    // If we get here then the connection is closed gracefully
    if (this->rpcPeer.getPeered() && !this->accountHash.empty()) {
        RpcSessionManager::getInstance()->removeAccountSessionMapping(this->accountHash);
    }

    if(ec)
        return fail(ec, "close");

    if (this->readQueuePtr) {
        this->readQueuePtr->deactivate();
        this->readQueuePtr.reset();
    }

    KETO_LOG_INFO << this->sessionNumber << ": Close the connection";
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
    ss << command << " " << Botan::hex_encode(message,true);
    return ss.str();
}

void RpcSession::closeResponse(const std::string& command, const std::string& message) {

    KETO_LOG_INFO << "Server closing connection because [" << message << "]";
    this->setClosed(true);
}

std::string RpcSession::helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash) {
    keto::asn1::HashHelper initHashHelper(initHash,keto::common::StringEncoding::HEX);
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,buildConsensus(initHashHelper));
}

std::string RpcSession::consensusSessionResponse(const std::string& command, const std::string& sessionKey) {
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION,"OK");
}

std::string RpcSession::consensusResponse(const std::string& command, const std::string& message) {
    keto::asn1::HashHelper hashHelper(message,keto::common::StringEncoding::HEX);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS,buildConsensus(hashHelper));
}

std::string RpcSession::serverRequest(const std::string& command, const std::vector<uint8_t>& message) {
    return serverRequest(command, Botan::hex_encode(message,true));
}

std::string RpcSession::serverRequest(const std::string& command, const std::string& message) {

    KETO_LOG_DEBUG << this->sessionNumber << ": Send the server request : " << command;
    std::stringstream ss;
    ss << buildMessage(command,message);
    KETO_LOG_DEBUG << this->sessionNumber << ": Sent the server request : " << command;
    return ss.str();
}

void RpcSession::handlePeerResponse(const std::string& command, const std::string& message) {
    std::string response = keto::server_common::VectorUtils().copyVectorToString(
        Botan::hex_decode(message,true));
    keto::rpc_protocol::PeerResponseHelper peerResponseHelper(response);
    
    RpcSessionManager::getInstance()->setPeers(peerResponseHelper.getPeers());
    KETO_LOG_INFO << this->sessionNumber << ": Received the list of peers will now reconnect to them : " <<
                                                                                                         peerResponseHelper.getPeers().size();

    // Read a message into our buffer
    this->setClosed(true);
}

std::string RpcSession::handleRegisterRequest(const std::string& command, const std::string& message) {

    // notify the accepted
    keto::router_utils::RpcPeerHelper rpcPeerHelper;
    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    rpcPeerHelper.setActive(RpcSessionManager::getInstance()->isActivated());

    keto::proto::RpcPeer rpcPeer = rpcPeerHelper;
    std::string rpcValue;
    rpcPeer.SerializePartialToString(&rpcValue);

    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REGISTER, Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true));
}

std::string RpcSession::handleTransaction(const std::string& command, const std::string& message) {
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
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED,Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

std::string RpcSession::handleBlock(const std::string& command, const std::string& message) {
    keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
    signedBlockWrapperMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
                            keto::server_common::Events::BLOCK_PERSIST_MESSAGE,signedBlockWrapperMessage)));

    std::string result = messageWrapperResponse.SerializeAsString();
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED, Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}


std::string RpcSession::handleBlockSyncRequest(const std::string& command, const std::string& payload) {
    KETO_LOG_DEBUG << "[RpcSession::handleBlockSyncRequest] handle the block sync request : " << command;
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
    KETO_LOG_INFO << "[RpcSession::handleBlockSyncRequest] Setup the block sync reply";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE, Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}


std::string RpcSession::handleBlockSyncResponse(const std::string& command, const std::string& message) {
    keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
    signedBlockBatchMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    keto::proto::MessageWrapperResponse messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                            keto::server_common::Events::BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage)));

    std::string result = messageWrapperResponse.SerializeAsString();
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_PROCESSED, Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
}

std::string RpcSession::handleProtocolCheckRequest(const std::string& command, const std::string& message) {

    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->resetProtocolCheck();

    keto::asn1::HashHelper initHashHelper(message,keto::common::StringEncoding::HEX);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE, buildConsensus(initHashHelper));
}


void RpcSession::handleProtocolCheckAccept(const std::string& command, const std::string& message) {

    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyProtocolCheck();

}

void RpcSession::handleProtocolHeartbeat(const std::string& command, const std::string& message) {
    keto::proto::ProtocolHeartbeatMessage protocolHeartbeatMessage;
    protocolHeartbeatMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));
    // clear out the election result cache every type a new election begins.
    electionResultCache.heartBeat(protocolHeartbeatMessage);
    keto::software_consensus::ConsensusSessionManager::getInstance()->initNetworkHeartbeat(protocolHeartbeatMessage);
}

std::string RpcSession::handleElectionRequest(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][handleElectionRequest] the elect request has been received";
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionResultMessage>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::ElectionPeerMessage>(
                            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_REQUEST,electionPeerMessageProtoHelper))));

    std::string result = electionResultMessageProtoHelper;
    KETO_LOG_DEBUG << this->sessionNumber << ": [RpcSession][handleElectionRequest] the election is complete return the result";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE,
                         Botan::hex_encode((uint8_t*)result.data(),result.size(),true));

}

void RpcSession::handleElectionResponse(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << "[RpcSession::handleElectionResponse] handle the election response : " << command;
    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::ElectionResultMessage>(
                    keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE,electionResultMessageProtoHelper));
    KETO_LOG_DEBUG << "[RpcSession::handleElectionResponse] handled the election response : " << command;
}


void RpcSession::handleElectionPublish(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << "[RpcSession::handleElectionPublish] handle election confirmation";
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper.getAccount())) {
        KETO_LOG_DEBUG << "[RpcSession::handleElectionPublish] ignore the publish request";
        return;
    }

    KETO_LOG_DEBUG << "[RpcSession::handleElectionPublish] election process publish";
    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_PUBLISH).
            publish(electionPublishTangleAccountProtoHelper);
}


void RpcSession::handleElectionConfirmation(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << "[RpcSession::handleElectionConfirmation] Handle election confirmation";
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        KETO_LOG_DEBUG << "[RpcSession::handleElectionConfirmation] ignoring confirmation";
        return;
    }

    KETO_LOG_DEBUG << "[RpcSession::handleElectionConfirmation] election process confirmation";
    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_CONFIRMATION).
            confirmation(electionConfirmationHelper);
}


std::string RpcSession::handleRegisterResponse(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << "[RpcSession::registerResponse] handle the registration response";
    keto::router_utils::RpcPeerHelper rpcPeerHelper(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    this->accountHash = rpcPeerHelper.getAccountHash();
    this->setActive(rpcPeerHelper.isActive());
    RpcSessionManager::getInstance()->setAccountSessionMapping(rpcPeerHelper.getAccountHashString(),
            shared_from_this());

    KETO_LOG_DEBUG << "[RpcSession::registerResponse] update the server state with peers";
    keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                keto::server_common::Events::REGISTER_RPC_PEER_SERVER,rpcPeerHelper));

    // set the registered flag
    this->registered = true;

    KETO_LOG_DEBUG << "[RpcSession::registerResponse] return the next command";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}

std::string RpcSession::handleRequestNetworkSessionKeys(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << "[RpcSession::handleRequestNeworkKeys] return the next command";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}

std::string RpcSession::handleRequestNetworkSessionKeysResponse(const std::string& command, const std::string& message) {
    // notify the accepted inorder to set the network keys
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyAccepted();

    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleRequestNetworkSessionKeysResponse]: Process the hex message";
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleRequestNetworkSessionKeysResponse]: Set the network session keys";
    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleRequestNetworkSessionKeysResponse]: Set request the master network keys";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS);
}


std::string RpcSession::handleRequestNetworkMasterKeyResponse(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleRequestNetworkMasterKeyResponse]: Process the master network key";
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleRequestNetworkMasterKeyResponse]: Request the network keys";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS);
}

std::string RpcSession::handleRequestNetworkKeysResponse(const std::string& command, const std::string& message) {
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_KEYS,networkKeysWrapper)));

    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES);
}

std::string RpcSession::handleRequestNetworkFeesResponse(const std::string& command, const std::string& message) {
    keto::transaction_common::FeeInfoMsgProtoHelper feeInfoMsgProtoHelper(
            Botan::hex_decode(message));

    keto::proto::FeeInfoMsg feeInfoMsg = feeInfoMsgProtoHelper;
    feeInfoMsg = keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::FeeInfoMsg>(
                            keto::server_common::Events::NETWORK_FEE_INFO::SET_NETWORK_FEE,feeInfoMsg)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkFeesResponse][" << this->getPeer().getHost() << "][" << this->sessionNumber << "] #######################################################";
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkFeesResponse][" << this->getPeer().getHost() << "][" << this->sessionNumber << "] ######## Network intialization is now complete ########";
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkFeesResponse][" << this->getPeer().getHost() << "][" << this->sessionNumber << "] #######################################################";


    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE,
                         keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE);
}

std::string RpcSession::handleRetryResponse(const std::string& command) {

    std::string result;
    KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Processing failed for the command : " << command;
    if (command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS ||
        command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS  ||
        command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS ||
        command == keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES) {

        KETO_LOG_DEBUG << "[RpcSession::handleRetryResponse]Process the retry response : " << command;
        std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));

        KETO_LOG_INFO << "[RpcSession::handleRetryResponse]Send the retry : " << command;
        result = serverRequest(command,command);
        KETO_LOG_DEBUG << "[RpcSession::handleRetryResponse]After sending the retry : " << command;
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS ||
               command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS  ||
               command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS ||
               command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) {

        // attempt to make a new request for the failed response processing.
        KETO_LOG_DEBUG << "[RpcSession::handleRetryResponse] Process the retry response : " << command;
        std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));

        std::string request = command;
        request.replace(0,std::string("RESPONSE").size(),"REQUEST");

        KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Send the retry : " << request;
        result = serverRequest(request,request);
        KETO_LOG_DEBUG << "[RpcSession::handleRetryResponse] After sending the retry : " << request;
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::ACCEPTED ||
               command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT ||
               command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS ||
               command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION ||
               command == keto::server_common::Constants::RPC_COMMANDS::HELLO ||
               command == keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS ||
               command == keto::server_common::Constants::RPC_COMMANDS::PEERS ||
               command == keto::server_common::Constants::RPC_COMMANDS::REGISTER ||
               command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST) {
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Attempt to reconnect";
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        this->setClosed(true);
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST ||
               command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) {
        // this indicates the up stream server is currently out of sync and cannot be relied upon we therefore
        // need to use an alternative and mark this one as inactive until it is activated.
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Deactive this session and re-schedule the retry";
        this->deactivate();

        // reschedule the block sync retry
        keto::proto::MessageWrapper messageWrapper;
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                keto::server_common::Events::BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY,messageWrapper));
    } else {
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Ignore as no retry is required";
        KETO_LOG_INFO << this->sessionNumber << ": Setup connection for read : " << command << std::endl;
    }

    return result;
}

void RpcSession::handleActivatePeer(const std::string& command, const std::string& message) {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleActivatePeer]: Activate the peer";
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message));
    keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);
    this->setActive(rpcPeerHelper.isActive());
    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ACTIVATE_RPC_PEER,rpcPeerHelper));
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::handleActivatePeer]: After activating the peer";
}

void RpcSession::activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {

    std::string rpcValue = rpcPeerHelper;
    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE, Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true)));
}

void RpcSession::routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
    std::string messageWrapperStr = messageWrapper.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
                  messageBytes));
}


void RpcSession::requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    std::string messageWrapperStr = signedBlockBatchRequest.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    KETO_LOG_INFO << "[RpcSession::requestBlockSync] Requesting block sync from [" << this->getPeer().getPeer() << "] : " <<
                  keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST;
    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                  messageBytes));


}


void RpcSession::pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::pushBlock]: push the block";
    std::string messageWrapperStr;
    signedBlockWrapperMessage.SerializeToString(&messageWrapperStr);
    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                  Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true)));
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::pushBlock]: ";
}


void
RpcSession::electBlockProducer() {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::electBlockProducer]: call the block producer";
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper;
    electionPeerMessageProtoHelper.setAccount(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPeerMessageProtoHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                       messageBytes));
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::electBlockProducer]: call the block ";
}

void
RpcSession::electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::electBlockProducerPublish]: publish the election result";
    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper.getAccount())) {
        return;
    }

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPublishTangleAccountProtoHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                       messageBytes));
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::electBlockProducerPublish]: published the election result";
}

void
RpcSession::electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::electBlockProducerConfirmation]: confirmation of the election results";
    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        return;
    }

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionConfirmationHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                       messageBytes));
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::electBlockProducerConfirmation]: confirmation sent for election results";
}


void
RpcSession::pushRpcPeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::pushToRpcPeer]: confirmation of the election results";

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            rpcPeerHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS,
                       messageBytes));
    KETO_LOG_DEBUG << this->sessionNumber << "[RpcSession::pushToRpcPeer]: confirmation sent for election results";
}


RpcPeer RpcSession::getPeer() {
    return this->rpcPeer;
}

bool RpcSession::isClosed() {
    return this->closed;
}

bool RpcSession::isActive() {
    return this->active;
}

bool RpcSession::isRegistered() {
    return this->registered;
}

void
RpcSession::send(const std::string& message) {
    net::post(
        ws_.get_executor(),
        beast::bind_front_handler(
            &RpcSession::sendMessage,
            shared_from_this(),
            std::make_shared<std::string>(message)));
}

void
RpcSession::sendMessage(std::shared_ptr<std::string> ss) {
    KETO_LOG_DEBUG << "[RpcSession::sendMessage][" << this->sessionNumber << "][sendMessage] : push an entry into the queue";

    // Always add to queue
    queue_.push_back(ss);

    // Are we already writing?
    if (queue_.size() > 1) {
        return;
    }

    sendFirstQueueMessage();
    KETO_LOG_DEBUG << "[RpcSession::sendMessage][" << this->sessionNumber << "][sendMessage] : send a message";
}

void
RpcSession::sendFirstQueueMessage() {
    KETO_LOG_DEBUG << "[RpcSession::sendMessage][" << this->sessionNumber << "][sendFirstQueueMessage] send the message from the message buffer :" <<
        keto::server_common::StringUtils(*queue_.front()).tokenize(" ")[0];

    // Send the message
    ws_.async_write(
            boost::asio::buffer(*queue_.front()),
            beast::bind_front_handler(
                    &RpcSession::on_write,
                    shared_from_this()));

    KETO_LOG_DEBUG << "[RpcServer][" << this->sessionNumber << "][sendFirstQueueMessage] after sending the message";
}


// change state
void RpcSession::setClosed(bool closed) {
    this->closed = closed;
}

void RpcSession::deactivate() {
    this->active = false;
}

void RpcSession::setActive(bool active) {
    this->active = active;
}

}
}
