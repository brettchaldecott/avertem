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
#include <boost/asio/ip/host_name.hpp>

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
#include "keto/rpc_protocol/PeerRequestHelper.hpp"
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
#include "keto/election_common/PublishedElectionInformationHelper.hpp"
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

RpcSession::ReadQueue::ReadQueue(RpcSession* session) : session(session), active(true) {
    queueThreadPtr = std::shared_ptr<std::thread>(new std::thread(
            [this]
            {
                this->run();
            }));
}

RpcSession::ReadQueue::~ReadQueue() {
    deactivate();
}

bool RpcSession::ReadQueue::isActive() {
    std::unique_lock<std::mutex> guard(classMutex);
    return this->active;
}

void RpcSession::ReadQueue::deactivate() {
    {
        std::unique_lock<std::mutex> guard(classMutex);
        if (!this->active) {
            return;
        }
        this->active = false;
        stateCondition.notify_all();
    }
    queueThreadPtr->join();
    queueThreadPtr.reset();
}

void RpcSession::ReadQueue::pushEntry(const std::string& command, const keto::server_common::StringVector& stringVector) {
    std::unique_lock<std::mutex> guard(classMutex);
    if (!this->active) {
        return;
    }
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
    while(this->isActive()) {
        RpcSession::ReadQueueEntryPtr entry = popEntry();
        if (entry) {
            this->session->processQueueEntry(entry);
        }
    }
}

// Report a failure
void
RpcSession::fail(boost::system::error_code ec, const std::string& what) {
    KETO_LOG_ERROR << "Failed processing : [" << rpcPeer.getHost() << "][" << isClosed() <<  "][" << what << ": " << ec.message();
}

std::string RpcSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSession::RpcSession(
        std::shared_ptr<net::io_context> ioc,
        std::shared_ptr<sslBeast::context> ctx,
        const RpcPeer& rpcPeer) :
        closed(false),
        active(false),
        registered(false),
        terminated(false),
        resolver(net::make_strand(*ioc)),
        ws_(net::make_strand(*ioc), *ctx),
        rpcPeer(rpcPeer),
        retryCount(0){
    this->sessionNumber = sessionIndex++;
    ws_.auto_fragment(false);
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


    // setup the external hostname information
    if (config->getVariablesMap().count(Constants::EXTERNAL_HOSTNAME)) {
        this->externalHostname =
            config->getVariablesMap()[Constants::EXTERNAL_HOSTNAME].as<std::string>();
    } else {
        this->externalHostname = boost::asio::ip::host_name();
    }

    this->readQueuePtr = ReadQueuePtr(new RpcSession::ReadQueue(this));
    RpcSessionManager::getInstance()->incrementSessionCount();
}

RpcSession::~RpcSession() {
    RpcSessionManager::getInstance()->decrementSessionCount();
    this->readQueuePtr.reset();
    this->terminated = true;
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
    if (RpcSessionManager::getInstance()->isTerminated()) {
        RpcSessionManager::getInstance()->removeSession(this->rpcPeer, this->accountHash);
        return;
    } else if(ec) {
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        return fail(ec, Constants::SESSION::RESOLVE);
    }

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
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        return fail(ec, Constants::SESSION::CONNECT);
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(60));

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
    if (RpcSessionManager::getInstance()->isTerminated()) {
        RpcSessionManager::getInstance()->removeSession(this->rpcPeer, this->accountHash);
        return;
    } else if(ec) {
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        return fail(ec, Constants::SESSION::SSL_HANDSHAKE);
    }

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
    if (RpcSessionManager::getInstance()->isTerminated()) {
        do_close();
        RpcSessionManager::getInstance()->removeSession(this->rpcPeer, this->accountHash);
        return;
    } else if(ec) {
        do_close();
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        return fail(ec, Constants::SESSION::HANDSHAKE);
    }

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
    boost::ignore_unused(bytes_transferred);
    queue_.pop_front();

    if(ec) {
        readQueuePtr->deactivate();
        return fail(ec, "write");
    }

    if (queue_.size()) {
        sendFirstQueueMessage();
    }
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

    if (this->isClosed()) {
        readQueuePtr->deactivate();
        return;
    } else if (ec && !ws_.is_open()) {
        readQueuePtr->deactivate();
        this->setClosed(true);
        RpcSessionManager::getInstance()->removeSession(this->rpcPeer, this->accountHash);
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        return;
    } else if (ec) {
        readQueuePtr->deactivate();
        closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE,keto::server_common::Constants::RPC_COMMANDS::CLOSE);
        RpcSessionManager::getInstance()->reconnect(rpcPeer);
        return fail(ec, "read");
    }

    boost::ignore_unused(bytes_transferred);

    // parse the input
    std::stringstream ss;
    ss << boost::beast::make_printable(buffer_.data());
    std::string data = ss.str();
    stringVector = keto::server_common::StringUtils(data).tokenize(" ");
    command = data;
    if (stringVector.size() > 1) {
        command = stringVector[0];
    }
    // Clear the buffer
    buffer_.consume(buffer_.size());

    this->readQueuePtr->pushEntry(command,stringVector);

    if (this->isClosed()) {
        KETO_LOG_INFO << this->sessionNumber << " Closing the session";
        return;
    } else {
        do_read();
    }
}


void RpcSession::processQueueEntry(const ReadQueueEntryPtr& readQueueEntryPtr) {
    if (RpcSessionManager::getInstance()->isTerminated()) {
        return;
    }
    std::string command = readQueueEntryPtr->getCommand();
    keto::server_common::StringVector stringVector = readQueueEntryPtr->getStringVector();
    std::string message;
    try {
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();

        // Close the WebSocket connection
        if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
            message = helloConsensusResponse(command, stringVector[1], stringVector[2]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY) == 0) {
            //closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE, stringVector[1]);
            handleRetryResponse(keto::server_common::Constants::RPC_COMMANDS::GO_AWAY);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED) == 0) {
            message = helloAcceptedResponse(keto::server_common::Constants::RPC_COMMANDS::ACCEPTED,stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
            // peer response requires this session is shut down
            handlePeerResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REGISTER) == 0) {
            message = handleRegisterResponse(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE) == 0) {
            handleActivatePeer(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
            message = handleTransaction(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK) == 0) {
            message = handleBlock(command, stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_PROCESSED) == 0) {
            // do nothing
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
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS) == 0) {
            message = handleRequestNetworkStatus(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS,
                                       stringVector[1]);
        } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS) == 0) {
            handleResponseNetworkStatus(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS,
                                                 stringVector[1]);
        }
        transactionPtr->commit();
    } catch (keto::common::Exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed to process because : " << boost::diagnostic_information_what(ex, true);
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry] cause : " << ex.what();
        message = handleInternalException(command,ex.what());
    } catch (boost::exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed to process because : " << boost::diagnostic_information_what(ex, true);
        message = handleInternalException(command);
    } catch (std::exception &ex) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed process the request : " << ex.what();
        message = handleInternalException(command);
    } catch (...) {
        KETO_LOG_ERROR << "[RPCSession::processQueueEntry][" << this->sessionNumber << "]" << command
                       << " : Failed process the request" << std::endl;
        message = handleInternalException(command);
    }

    if (!message.empty() && !this->isClosed()) {
        send(message);
    }
}


void
RpcSession::do_close() {

    // Close the WebSocket connection
    if (!ws_.is_open()) {
        // ignore a closed connection
        return;
    }
    ws_.async_close(websocket::close_code::normal,
                    beast::bind_front_handler(
                            &RpcSession::on_close,
                            shared_from_this()));
}

void
RpcSession::on_close(boost::system::error_code ec)
{
    // deactivate the queue
    KETO_LOG_INFO << this->sessionNumber << ": closing the connection";
    readQueuePtr->deactivate();

    // remove from the peered list
    if (this->rpcPeer.getPeered()) {
        keto::router_utils::RpcPeerHelper rpcPeerHelper;
        rpcPeerHelper.setAccountHash(this->accountHash);
        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                        keto::server_common::Events::DEREGISTER_RPC_PEER,rpcPeerHelper));
    }

    if(ec)
        return fail(ec, "close");

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
    send(keto::server_common::Constants::RPC_COMMANDS::CLOSE);
    this->setClosed(true);
    RpcSessionManager::getInstance()->removeSession(this->rpcPeer, this->accountHash);
}

std::string RpcSession::helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    keto::asn1::HashHelper initHashHelper(initHash,keto::common::StringEncoding::HEX);
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    // guarantee order of consensus handling to prevent sessions from getting incorrectly setup
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS,buildConsensus(initHashHelper));
}

std::string RpcSession::consensusSessionResponse(const std::string& command, const std::string& sessionKey) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    keto::crypto::SecureVector initVector = Botan::hex_decode_locked(sessionKey,true);
    keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION,"OK");
}

std::string RpcSession::consensusResponse(const std::string& command, const std::string& message) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    keto::asn1::HashHelper hashHelper(message,keto::common::StringEncoding::HEX);
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS,buildConsensus(hashHelper));
}

std::string RpcSession::helloAcceptedResponse(const std::string& command, const std::string& message) {
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
    if (!this->rpcPeer.getPeered()) {
        return handlePeerRequest(command, message);
    }

    // notify the accepted inorder to set the network keys
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkSessionKeysResponse] handle network session accepted";
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyAccepted();

    if (this->isRegistered()) {
        return handleRequestNetworkSessionKeys(command, message);
    } else {
        return handleRegisterRequest(command, message);
    }
}

std::string RpcSession::serverRequest(const std::string& command, const std::vector<uint8_t>& message) {
    return serverRequest(command, Botan::hex_encode(message,true));
}

std::string RpcSession::serverRequest(const std::string& command, const std::string& message) {

    std::stringstream ss;
    ss << buildMessage(command,message);
    return ss.str();
}

std::string RpcSession::handlePeerRequest(const std::string& command, const std::string& message) {
    keto::rpc_protocol::PeerRequestHelper peerRequestHelper;

    peerRequestHelper.addAccountHash(this->accountHash);
    peerRequestHelper.setHostname(this->externalHostname);

    keto::proto::PeerRequest peerRequest = peerRequestHelper;
    std::string peerRequestValue;
    peerRequest.SerializePartialToString(&peerRequestValue);

    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::PEERS, Botan::hex_encode((uint8_t*)peerRequestValue.data(),peerRequestValue.size(),true));
}

void RpcSession::handlePeerResponse(const std::string& command, const std::string& message) {
    std::string response = keto::server_common::VectorUtils().copyVectorToString(
        Botan::hex_decode(message,true));
    keto::rpc_protocol::PeerResponseHelper peerResponseHelper(response);

    // Read a message into our buffer
    closeResponse(command,command);
    RpcSessionManager::getInstance()->setPeers(peerResponseHelper.getPeers(),true);
    KETO_LOG_INFO << this->sessionNumber << ": Received the list of peers will now reconnect to them : " <<
                                                                                                         peerResponseHelper.getPeers().size();
}

std::string RpcSession::handleRegisterRequest(const std::string& command, const std::string& message) {

    // peer request helper
    keto::rpc_protocol::PeerRequestHelper peerRequestHelper;
    peerRequestHelper.addAccountHash(this->accountHash);
    peerRequestHelper.setHostname(this->externalHostname);
    keto::proto::PeerRequest peerRequest = peerRequestHelper;
    std::string peerRequestValue;
    peerRequest.SerializePartialToString(&peerRequestValue);

    // notify the accepted
    keto::router_utils::RpcPeerHelper rpcPeerHelper;
    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    rpcPeerHelper.setActive(RpcSessionManager::getInstance()->isActivated());
    keto::proto::RpcPeer rpcPeer = rpcPeerHelper;
    std::string rpcValue;
    rpcPeer.SerializePartialToString(&rpcValue);

    // setup the registration response including the mising peer request and rpc information
    std::stringstream sstream;
    sstream <<  Botan::hex_encode((uint8_t*)peerRequestValue.data(),peerRequestValue.size(),true) << "#" <<
        Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true);

    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REGISTER, sstream.str());
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
    std::unique_lock<std::recursive_mutex> uniqueLock(keto::software_consensus::ConsensusSessionManager::getInstance()->getMutex());
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
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionResultMessage>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::ElectionPeerMessage>(
                            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_REQUEST,electionPeerMessageProtoHelper))));

    std::string result = electionResultMessageProtoHelper;
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE,
                         Botan::hex_encode((uint8_t*)result.data(),result.size(),true));

}

void RpcSession::handleElectionResponse(const std::string& command, const std::string& message) {
    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::ElectionResultMessage>(
                    keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE,electionResultMessageProtoHelper));
}


void RpcSession::handleElectionPublish(const std::string& command, const std::string& message) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper.getAccount())) {
        return;
    }

    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_PUBLISH).
            publish(electionPublishTangleAccountProtoHelper);
}


void RpcSession::handleElectionConfirmation(const std::string& command, const std::string& message) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    // prevent echo propergation at the boundary
    if (this->electionResultCache.containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        return;
    }

    keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_CONFIRMATION).
            confirmation(electionConfirmationHelper);
}


std::string RpcSession::handleRequestNetworkStatus(const std::string& command, const std::string& message) {
    keto::proto::PublishedElectionInformation publishedElectionInformation;
    publishedElectionInformation =
            keto::server_common::fromEvent<keto::proto::PublishedElectionInformation>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
                            keto::server_common::Events::ROUTER_QUERY::GET_PUBLISHED_ELECTION_INFO,publishedElectionInformation)));

    std::string result = publishedElectionInformation.SerializeAsString();
    std::stringstream ss;
    ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS
       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
    return ss.str();
}


void RpcSession::handleResponseNetworkStatus(const std::string& command, const std::string& message) {
    if (RpcSessionManager::getInstance()->hasNetworkState()) {
        return;
    }
    keto::election_common::PublishedElectionInformationHelper publishedElectionInformationHelper(
            keto::server_common::VectorUtils().copyVectorToString(
                    Botan::hex_decode(message)));

    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
                            keto::server_common::Events::ROUTER_QUERY::SET_PUBLISHED_ELECTION_INFO,publishedElectionInformationHelper));
    RpcSessionManager::getInstance()->activateNetworkState();
}


std::string RpcSession::handleRegisterResponse(const std::string& command, const std::string& message) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message)));

    this->accountHash = rpcPeerHelper.getAccountHash();
    this->setActive(rpcPeerHelper.isActive());
    RpcSessionManager::getInstance()->setAccountSessionMapping(rpcPeerHelper.getAccountHashString(),
            shared_from_this());

    keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                keto::server_common::Events::REGISTER_RPC_PEER_SERVER,rpcPeerHelper));

    // set the registered flag
    this->registered = true;

    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}

std::string RpcSession::handleRequestNetworkSessionKeys(const std::string& command, const std::string& message) {
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS,
                         keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS);
}

std::string RpcSession::handleRequestNetworkSessionKeysResponse(const std::string& command, const std::string& message) {
    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkSessionKeysResponse] set the network session keys";
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkSessionKeysResponse] request the master network keys";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS);
}


std::string RpcSession::handleRequestNetworkMasterKeyResponse(const std::string& command, const std::string& message) {
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkMasterKeyResponse] Set the master keys";
    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkMasterKeyResponse] After setting the master keys";
    return serverRequest(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS,
                  keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS);
}

std::string RpcSession::handleRequestNetworkKeysResponse(const std::string& command, const std::string& message) {
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(
            Botan::hex_decode(message));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkKeysResponse] Set the network keys";
    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
            keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                            keto::server_common::Events::SET_NETWORK_KEYS,networkKeysWrapper)));

    KETO_LOG_INFO << "[RpcSession::handleRequestNetworkKeysResponse] After setting the network keys";
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


std::string RpcSession::handleInternalException(const std::string& command, const std::string& cause) {

    std::string result;
    KETO_LOG_INFO << "[RpcSession::handleInternalException] Processing failed for the command : " << command;
    if (command == keto::server_common::Constants::RPC_COMMANDS::ACCEPTED ||
            command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS ||
            command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS  ||
            command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS ||
            command == keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES) {

        // force the session
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << this->getPeer().getHost() << "][" << command << "] reconnect to the server";
        if (cause == "Invalid Session exception." || cause == "Invalid password exception." || cause.find("Index out of bounds") != std::string::npos) {
            KETO_LOG_INFO << "[RpcSession::handleInternalException][" << this->getPeer().getHost() << "][" << command << "] force a reset of the session as it is currently invalid";
            keto::software_consensus::ConsensusSessionManager::getInstance()->resetSessionKey();
        }

        if (!RpcSessionManager::getInstance()->isTerminated()) {
            closeResponse(command,command);
            RpcSessionManager::getInstance()->reconnect(rpcPeer);
        }
        result = keto::server_common::Constants::RPC_COMMANDS::CLOSED;

    } else if (command == keto::server_common::Constants::RPC_COMMANDS::HELLO ||
               command == keto::server_common::Constants::RPC_COMMANDS::GO_AWAY ||
               command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT ||
               command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS ||
               command == keto::server_common::Constants::RPC_COMMANDS::CONSENSUS_SESSION ||
               command == keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS ||
               command == keto::server_common::Constants::RPC_COMMANDS::PEERS ||
               command == keto::server_common::Constants::RPC_COMMANDS::REGISTER ||
               command == keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST) {
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << this->getPeer().getHost() << "] Attempt to reconnect";
        if (!RpcSessionManager::getInstance()->isTerminated()) {
            closeResponse(command,command);
            RpcSessionManager::getInstance()->reconnect(rpcPeer);
        }
        result = keto::server_common::Constants::RPC_COMMANDS::CLOSED;
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) {
        // this indicates the up stream server is currently out of sync and cannot be relied upon we therefore
        // need to use an alternative and mark this one as inactive until it is activated.
        KETO_LOG_INFO << "[RpcSession::handleInternalException][" << this->getPeer().getHost() << "] Deactive this session and re-schedule the retry";
        this->deactivate();
        // reschedule the block sync retry
        keto::proto::MessageWrapper messageWrapper;
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                keto::server_common::Events::BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY,messageWrapper));
    } else {
        KETO_LOG_INFO << "[RpcSession::handleInternalException] Ignore as no retry is required";
        KETO_LOG_INFO << this->sessionNumber << ": Setup connection for read : " << command << std::endl;
    }

    return result;
}

std::string RpcSession::handleRetryResponse(const std::string& command) {

    std::string result;
    KETO_LOG_INFO << "[RpcSession::handleRetryResponse] Processing failed for the command : " << command;
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
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse][" << this->getPeer().getHost() << "] Attempt to reconnect";
        if (!RpcSessionManager::getInstance()->isTerminated()) {
            closeResponse(command, command);
            RpcSessionManager::getInstance()->reconnect(rpcPeer);
        }
        result = keto::server_common::Constants::RPC_COMMANDS::CLOSED;
    } else if  (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK) {
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse][" << this->getPeer().getHost() << "] Attempt to reconnect to the upstream to force a session key update";
        if (!RpcSessionManager::getInstance()->isTerminated()) {
            closeResponse(command, command);
            RpcSessionManager::getInstance()->reconnect(rpcPeer);
        }
        result = keto::server_common::Constants::RPC_COMMANDS::CLOSED;
    } else if (command == keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) {
        // this indicates the up stream server is currently out of sync and cannot be relied upon we therefore
        // need to use an alternative and mark this one as inactive until it is activated.
        KETO_LOG_INFO << "[RpcSession::handleRetryResponse][" << this->getPeer().getHost() << "] Block sync requires a retry reschedule";

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
    std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
            Botan::hex_decode(message));
    keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);
    this->setActive(rpcPeerHelper.isActive());
    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ACTIVATE_RPC_PEER,rpcPeerHelper));
}

void RpcSession::activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    std::string rpcValue = rpcPeerHelper;
    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE, Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true)));
}

void RpcSession::routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    std::string messageWrapperStr = messageWrapper.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
                  messageBytes));
}


void RpcSession::requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    std::string messageWrapperStr = signedBlockBatchRequest.SerializeAsString();
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            messageWrapperStr);

    KETO_LOG_INFO << "[RpcSession::requestBlockSync] Requesting block sync from [" << this->getPeer().getPeer() << "] : " <<
                  keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST;
    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                  messageBytes));


}


void RpcSession::pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    std::string messageWrapperStr;
    signedBlockWrapperMessage.SerializeToString(&messageWrapperStr);
    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                  Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true)));
}


void
RpcSession::closeSession() {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    closeResponse(keto::server_common::Constants::RPC_COMMANDS::CLOSE,keto::server_common::Constants::RPC_COMMANDS::CLOSE);
}

void
RpcSession::electBlockProducer() {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper;
    electionPeerMessageProtoHelper.setAccount(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPeerMessageProtoHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                       messageBytes));
}

void
RpcSession::electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    // prevent echo propergation at the boundary
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper.getAccount())) {
        return;
    }

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionPublishTangleAccountProtoHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                       messageBytes));
}

void
RpcSession::electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
    // prevent echo propergation at the boundary
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    if (this->electionResultCache.containsConfirmationAccount(electionConfirmationHelper.getAccount())) {
        return;
    }

    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            electionConfirmationHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                       messageBytes));
}


void
RpcSession::pushRpcPeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    if (this->closed) {
        return;
    }
    std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
            rpcPeerHelper);

    send(serverRequest(keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS,
                       messageBytes));
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
    // Always add to queue
    queue_.push_back(ss);

    // Are we already writing?
    if (queue_.size() > 1) {
        return;
    }

    sendFirstQueueMessage();
}

void
RpcSession::sendFirstQueueMessage() {
    // Send the message
    std::string message = *queue_.front();
    if (message == keto::server_common::Constants::RPC_COMMANDS::CLOSE) {
        do_close();
    } else {
        ws_.async_write(
                boost::asio::buffer(*queue_.front()),
                beast::bind_front_handler(
                        &RpcSession::on_write,
                        shared_from_this()));
    }
}


// change state
void RpcSession::setClosed(bool closed) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    this->closed = closed;
}

void RpcSession::deactivate() {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    this->active = false;
}

void RpcSession::setActive(bool active) {
    std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
    this->active = active;
}


void RpcSession::terminate() {
    this->terminated = true;
}

bool RpcSession::isTerminated() {
    return this->terminated;
}

}
}
