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

#include "SoftwareConsensus.pb.h"
#include "HandShake.pb.h"
#include "Route.pb.h"
#include "BlockChain.pb.h"
#include "Protocol.pb.h"
#include "KeyStore.pb.h"

#include <boost/beast/core.hpp>

#include <botan/hex.h>
#include <botan/base64.h>
#include <google/protobuf/message_lite.h>

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


using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace beastSsl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

namespace keto {
namespace rpc_server {


class BufferCache {
public:
    BufferCache() {

    }
    BufferCache(const BufferCache& orig) = delete;
    virtual ~BufferCache() {
        for (boost::beast::multi_buffer* buffer : buffers) {
            delete buffer;
        }
        buffers.clear();
    }

    boost::beast::multi_buffer* create() {
        boost::beast::multi_buffer* buffer = new boost::beast::multi_buffer();
        this->buffers.insert(buffer);
        return buffer;
    }
    void remove(boost::beast::multi_buffer* buffer) {
        this->buffers.erase(buffer);
        delete buffer;
    }
private:
    std::set<boost::beast::multi_buffer*> buffers;
};
typedef std::shared_ptr<BufferCache> BufferCachePtr;

class BufferScope {
public:
    BufferScope(const BufferCachePtr& bufferCachePtr, boost::beast::multi_buffer* buffer) :
        bufferCachePtr(bufferCachePtr), buffer(buffer) {
    }
    BufferScope(const BufferScope& orig) = delete;
    virtual ~BufferScope() {
        bufferCachePtr->remove(buffer);
    }

private:
    BufferCachePtr bufferCachePtr;
    boost::beast::multi_buffer* buffer;
};


    static RpcServerPtr singleton;

// Report a failure
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

class session;
typedef std::shared_ptr<session> sessionPtr;

class AccountSessionCache;
typedef std::shared_ptr<AccountSessionCache> AccountSessionCachePtr;

static AccountSessionCachePtr accountSessionSingleton;

class AccountSessionCache {
private:
    std::recursive_mutex classMutex;
    std::map<std::string,sessionPtr> accountSessionMap;

public:
    AccountSessionCache() {
        
    }
    
    AccountSessionCache(const AccountSessionCache& orig) = delete;
    
    ~AccountSessionCache() {
        
    }
    
    static AccountSessionCachePtr init() {
        return accountSessionSingleton = std::shared_ptr<AccountSessionCache>(
                new AccountSessionCache());
    }
    static void fin() {
        accountSessionSingleton.reset();
    }
    static AccountSessionCachePtr getInstance() {
        return accountSessionSingleton;
    }
    
    void addAccount(const std::string& account, 
            const sessionPtr sessionRef) {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        accountSessionMap[account] = sessionRef;
    }
    
    void removeAccount(const std::string& account,const session* session) {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        if (!accountSessionMap.count(account)) {
            return;
        } else if (accountSessionMap[account].get() != session) {
            return;
        }
        accountSessionMap.erase(account);
    }
    
    bool hasSession(const std::string& account) {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        if (accountSessionMap.count(account)) {
            return true;
        }
        return false;
    }
    
    sessionPtr getSession(const std::string& account) {
        std::lock_guard<std::recursive_mutex> guard(this->classMutex);
        return this->accountSessionMap[account];
    }

    std::vector<std::string> getSessions() {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        std::vector<std::string> keys;
        for(std::map<std::string,sessionPtr>::iterator it = this->accountSessionMap.begin();
            it != this->accountSessionMap.end(); ++it) {
            keys.push_back(it->first);
        }
        return keys;
    }
    
};


// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
private:
    std::recursive_mutex classMutex;
    tcp::socket socket_;
    websocket::stream<beastSsl::stream<tcp::socket&>> ws_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::multi_buffer buffer_;
    RpcServer* rpcServer;
    std::shared_ptr<keto::rpc_protocol::ServerHelloProtoHelper> serverHelloProtoHelperPtr;
    keto::crypto::SecureVector sessionId;
    std::shared_ptr<Botan::AutoSeeded_RNG> generatorPtr;


public:
    // Take ownership of the socket
    session(tcp::socket socket, beastSsl::context& ctx, RpcServer* rpcServer)
        : socket_(std::move(socket))
        , ws_(socket_, ctx)
        , strand_(ws_.get_executor())
        , rpcServer(rpcServer)
    {
        ws_.auto_fragment(false);
        this->generatorPtr = std::shared_ptr<Botan::AutoSeeded_RNG>(new Botan::AutoSeeded_RNG());
    }
        
    virtual ~session() {
        removeFromCache();
    }

    keto::crypto::SecureVector getSessionId() {
        return this->sessionId;
    }

    // Start the asynchronous operation
    void
    run()
    {
        // Perform the SSL handshake
        ws_.next_layer().async_handshake(
            beastSsl::stream_base::server,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_handshake,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void
    on_handshake(boost::system::error_code ec)
    {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        if(ec)
            return fail(ec, "handshake");

        // Accept the websocket handshake
        ws_.async_accept(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void
    on_accept(boost::system::error_code ec)
    {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        if(ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void
    removeFromCache() {
        if (serverHelloProtoHelperPtr) {
            std::string accountHash = keto::server_common::VectorUtils().copyVectorToString(
                    serverHelloProtoHelperPtr->getAccountHash());
            KETO_LOG_INFO << "End of server session remove it from the account session cache : " <<
                Botan::hex_encode(serverHelloProtoHelperPtr->getAccountHash());
            AccountSessionCache::getInstance()->removeAccount(accountHash,this);
        }
    }

    std::string
    getAccount() {
        if (serverHelloProtoHelperPtr) {
            return Botan::hex_encode(
                    serverHelloProtoHelperPtr->getAccountHash());
        }
        return "";
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        keto::server_common::StringVector stringVector;

        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed || !ws_.is_open()) {
            removeFromCache();
            return;
        }
        if (ec) {
            fail(ec, "read");
        }


        std::stringstream ss;
        ss << boost::beast::buffers(buffer_.data());
        std::string data = ss.str();
        stringVector = keto::server_common::StringUtils(data).tokenize(" ");


        // Clear the buffer
        buffer_.consume(buffer_.size());

        std::string command = stringVector[0];
        std::string payload;
        if (stringVector.size() == 2) {
            payload = stringVector[1];
        }
        std::string message;
        
        try {
            KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] process the command : " << command;
            keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
            if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO) == 0) {
                handleHello(command, payload);
                KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " Said hello";
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
                handleHelloConsensus(command, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
                KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " requested peers";
                handlePeer(command,payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REGISTER) == 0) {
                KETO_LOG_INFO << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " register";
                handleRegister(keto::server_common::Constants::RPC_COMMANDS::REGISTER, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
                KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] handle a transaction";
                handleTransaction(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION, payload);
                KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] Transaction processing complete";
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
                handleTransactionProcessed(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {

            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE) == 0) {

            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ROUTE_UPDATE) == 0) {

            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::SERVICES) == 0) {

            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
                // implement
                KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] close the session";
                AccountSessionCache::getInstance()->removeAccount(
                    keto::server_common::VectorUtils().copyVectorToString(    
                        serverHelloProtoHelperPtr->getAccountHash()),this);
                return;
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS) == 0) {
                handleRequestNetworkSessionKeys(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS) == 0) {
                handleRequestMasterNetworkKeys(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS) == 0) {
                handleRequestNetworkKeys(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES) == 0) {
                handleRequestNetworkFees(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK) == 0) {
                handleBlockPush(keto::server_common::Constants::RPC_COMMANDS::BLOCK, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) == 0) {
                handleBlockSyncRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST, payload);
            } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE) == 0) {
                handleProtocolCheckResponse(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE, payload);
            }
            KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] processed the command : " << command;
            transactionPtr->commit();
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][on_read] Failed to handle the request on the server [keto::common::Exception]: " << boost::diagnostic_information(ex,true);
            KETO_LOG_ERROR << "[RpcServer][on_read] Cause: " << boost::diagnostic_information(ex,true);
            handleRetryResponse(command);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][on_read] Failed to handle the request on the server [boost::exception]: " << boost::diagnostic_information(ex,true);
            KETO_LOG_ERROR << "[RpcServer][on_read] Failed to process because : " << boost::diagnostic_information(ex,true);
            handleRetryResponse(command);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][on_read] Failed to handle the request on the server [std::exception]: " << ex.what();
            KETO_LOG_ERROR << "Failed process the request : " << ex.what();
            handleRetryResponse(command);
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer][on_read] Failed to handle the request on the server [...]: unknown " << std::endl;
            KETO_LOG_ERROR << "[RpcServer][on_read] Failed to process : ";
            handleRetryResponse(command);
        }

        if (!ws_.is_open()) {
            removeFromCache();
            return;
        }
        ws_.text(ws_.got_text());
        ws_.async_write(
                buffer_.data(),
                boost::asio::bind_executor(
                        strand_,
                        std::bind(
                                &session::on_write,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2)));
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] Finished processing.";
    }
    
    void
    routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {

        if (!ws_.is_open()) {
            removeFromCache();
            BOOST_THROW_EXCEPTION(ConnectionLost());
        }
        std::string messageWrapperStr;
        messageWrapper.SerializeToString(&messageWrapperStr);
        MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
        clientRequest(multiBufferPtr, keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
                      Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
    }

    void
    pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
        KETO_LOG_INFO << "Attempt to push the block";
        if (!ws_.is_open()) {
            removeFromCache();
            return;
        }
        std::string messageWrapperStr;
        signedBlockWrapperMessage.SerializeToString(&messageWrapperStr);
        MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
        clientRequest(multiBufferPtr, keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                      Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
        KETO_LOG_INFO << "After writing the block";
    }

    //
    //
    // the protocol methods
    void
    performNetworkSessionReset() {
        KETO_LOG_INFO << "Attempt to perform the protocol reset via forcing the client to say hello";
        std::lock_guard<std::recursive_mutex> guard(classMutex);

        if (!ws_.is_open()) {
            removeFromCache();
            return;
        }
        // different message format
        MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
        boost::beast::ostream(*multiBufferPtr) << keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS
                                               << " " << Botan::hex_encode(this->rpcServer->getSecret())
                                               << " " << Botan::hex_encode(this->generateSession());
        ws_.text(ws_.got_text());
        ws_.async_write(
                multiBufferPtr->data(),
                boost::asio::bind_executor(
                        strand_,
                        std::bind(
                                &session::on_outBoundWrite,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2,
                                multiBufferPtr)));
        KETO_LOG_INFO << "After requesting the protocol reset from peers";
    }


    void
    performProtocolCheck() {
        KETO_LOG_INFO << "Attempt to perform the protocol check";

        if (!ws_.is_open()) {
            removeFromCache();
            return;
        }
        MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
        clientRequest(multiBufferPtr, keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST,
                      Botan::hex_encode(this->generateSession()));

        KETO_LOG_INFO << "After requesting the protocol check from peers";
    }



    void
    performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage) {
        KETO_LOG_INFO << "Attempt to perform the protocol heartbeat";
        if (!ws_.is_open()) {
            removeFromCache();
            return;
        }
        std::string messageWrapperStr;
        protocolHeartbeatMessage.SerializeToString(&messageWrapperStr);
        MultiBufferPtr multiBufferPtr(new boost::beast::multi_buffer());
        clientRequest(multiBufferPtr, keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_HEARTBEAT,
                Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
        KETO_LOG_INFO << "After requesting the protocol heartbeat";
    }

    void
    handleProtocolCheckResponse(const std::string& command, const std::string& payload) {
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
            boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT
                                           << " " << keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT;
            KETO_LOG_INFO << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was accepted";
        } else {
            boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY
                                           << " " << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY;
            KETO_LOG_INFO << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was rejected from network";
        }
    }

    void clientRequest(MultiBufferPtr multiBufferPtr, const std::string& command, const std::string& message) {

        std::lock_guard<std::recursive_mutex> guard(classMutex);
        boost::beast::ostream(*multiBufferPtr) <<
                                               buildMessage(command,message);
        KETO_LOG_INFO << "[RpcServer] Send the server request : " << command;
        ws_.text(ws_.got_text());
        ws_.async_write(
                multiBufferPtr->data(),
                boost::asio::bind_executor(
                        strand_,
                        std::bind(
                                &session::on_outBoundWrite,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2,
                                multiBufferPtr)));

        KETO_LOG_INFO << "[RpcServer] Sent the server request : " << command;
    }


    void
    on_outBoundWrite(
        boost::system::error_code ec,
        std::size_t bytes_transferred,
        MultiBufferPtr multiBufferPtr)
    {
        std::lock_guard<std::recursive_mutex> guard(classMutex);

        boost::ignore_unused(bytes_transferred);

        if(ec) {
            if (serverHelloProtoHelperPtr) {
                std::string accountHash = keto::server_common::VectorUtils().copyVectorToString(    
                    serverHelloProtoHelperPtr->getAccountHash());
                if (AccountSessionCache::getInstance()->hasSession(accountHash)) {
                    AccountSessionCache::getInstance()->removeAccount(accountHash,this);
                }
            }
            return fail(ec, "write");
        }

        // Clear the buffer
        multiBufferPtr->consume(multiBufferPtr->size());

    }
    
    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");
        
        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
    
    std::string buildMessage(const std::string& command, const std::string& message) {
        std::stringstream ss;
        ss << command << " " << message;
        return ss.str();
    }
    
    std::string buildMessage(const std::string& command, const std::vector<uint8_t>& message) {
        std::stringstream ss;
        ss << command << " " << Botan::hex_encode(message);
        return ss.str();
    }
    
    void handleHello(const std::string& command, const std::string& payload) {
        std::string bytes = keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload));
        this->serverHelloProtoHelperPtr = 
                std::shared_ptr<keto::rpc_protocol::ServerHelloProtoHelper>(
                new keto::rpc_protocol::ServerHelloProtoHelper(bytes));
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS
                << " " << Botan::hex_encode(this->rpcServer->getSecret()) 
                << " " << Botan::hex_encode(
                this->generateSession());
    }
    
    void handleHelloConsensus(const std::string& command, const std::string& payload) {
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
            boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::ACCEPTED
                << " " << keto::server_common::Constants::RPC_COMMANDS::ACCEPTED;
            KETO_LOG_INFO << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was accepted";
        } else {
            boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY 
                << " " << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY;
            KETO_LOG_INFO << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was rejected from network";
        }
    }
    
    void handlePeer(const std::string& command, const std::string& payload) {
        // first check if the external ip address has been added
        this->rpcServer->setExternalIp(this->socket_.local_endpoint().address());
        std::vector<std::string> urls;
        keto::rpc_protocol::PeerResponseHelper peerResponseHelper;
        if (payload == keto::server_common::Constants::RPC_COMMANDS::PEERS) {
            std::stringstream str;
            str << this->socket_.remote_endpoint().address().to_string() << ":" << Constants::DEFAULT_PORT_NUMBER;
            RpcServerSession::getInstance()->addPeer(
                this->serverHelloProtoHelperPtr->getAccountHash(),str.str());
            std::vector<std::string> peers = RpcServerSession::getInstance()->getPeers(this->serverHelloProtoHelperPtr->getAccountHash());
            peerResponseHelper.addPeers(peers);
            
        } else {
            
            // deserialize the object
        }
        std::string result;
        peerResponseHelper.operator keto::proto::PeerResponse().SerializePartialToString(&result);
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::PEERS 
                << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        
    }
    
    void handleRegister(const std::string& command, const std::string& payload) {
        
        // add this session for call backs
        AccountSessionCache::getInstance()->addAccount(
            keto::server_common::VectorUtils().copyVectorToString(    
            serverHelloProtoHelperPtr->getAccountHash()),
                shared_from_this());
        
        std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload));
        keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);
        
        keto::proto::RpcPeer rpcPeer = (keto::proto::RpcPeer)rpcPeerHelper;
        
        rpcPeer = keto::server_common::fromEvent<keto::proto::RpcPeer>(
                    keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::REGISTER_RPC_PEER,rpcPeer)));

        
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::REGISTER
                << " " << Botan::hex_encode(
                keto::server_common::ServerInfo::getInstance()->getAccountHash());
    }
    
    
    void handleTransaction(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleTransaction] handle transaction";
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
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED
                << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);

        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleTransaction] process the transaction";
    }
    
    void handleTransactionProcessed(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleTransactionProcessed] handle the transaction";

        keto::proto::MessageWrapperResponse messageWrapperResponse;
        messageWrapperResponse.ParseFromString(
            keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload)));

        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] transaction processed by peer [" <<
                this->serverHelloProtoHelperPtr->getAccountHashStr() << "] : " 
                << messageWrapperResponse.result();
        
    }

    void handleRequestNetworkSessionKeys(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestNetworkSessionKeys] << request the network session keys :" << command;

        keto::proto::NetworkKeysWrapper networkKeysWrapper;
        networkKeysWrapper =
                keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                                keto::server_common::Events::GET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

        std::string result = networkKeysWrapper.SerializeAsString();
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestNetworkSessionKeys] << after handling the network session keys request :" << command;
    }

    void handleRequestMasterNetworkKeys(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestMasterNetworkKeys] << request the master network keys :" << command;

        keto::proto::NetworkKeysWrapper networkKeysWrapper;
        networkKeysWrapper =
                keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                                keto::server_common::Events::GET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

        std::string result = networkKeysWrapper.SerializeAsString();
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestMasterNetworkKeys] << after handling the master network keys :" << command;
    }

    void handleRequestNetworkKeys(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] << request the network key :" << command;

        keto::proto::NetworkKeysWrapper networkKeysWrapper;
        networkKeysWrapper =
                keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                                keto::server_common::Events::GET_NETWORK_KEYS,networkKeysWrapper)));

        std::string result = networkKeysWrapper.SerializeAsString();
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);

        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] << after handling the request:" << command;

    }


    void handleRequestNetworkFees(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestNetworkFees] handle request network fees";
        keto::proto::FeeInfoMsg feeInfoMsg;
        feeInfoMsg =
                keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::FeeInfoMsg>(
                                keto::server_common::Events::NETWORK_FEE_INFO::GET_NETWORK_FEE,feeInfoMsg)));

        std::string result = feeInfoMsg.SerializeAsString();
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleRequestNetworkFees] request network fees";
    }

    void handleBlockPush(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleBlockPush] handle block push";
        keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
        std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload));
        signedBlockWrapperMessage.ParseFromString(rpcVector);

        keto::proto::MessageWrapperResponse messageWrapperResponse =
                keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
                                keto::server_common::Events::BLOCK_PERSIST_MESSAGE,signedBlockWrapperMessage)));
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleBlockPush] pushed the block";

    }

    void handleBlockSyncRequest(const std::string& command, const std::string& payload) {
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] handle the block sync request : " << command;
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
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] The block data returned [" << result.size() << "]";
    }

    void handleRetryResponse(const std::string& command) {
        boost::beast::ostream(buffer_) << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_RETRY
                                       << " " << command;
    }

    keto::crypto::SecureVector generateSession() {
        return this->sessionId = this->generatorPtr->random_vec(Constants::SESSION_ID_LENGTH);
    }

};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    std::shared_ptr<beastSsl::context> ctx_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    RpcServer* rpcServer;

public:
    listener(
        std::shared_ptr<boost::asio::io_context> ioc,
        std::shared_ptr<beastSsl::context> ctx,
        tcp::endpoint endpoint,
        RpcServer* rpcServer)
        : ctx_(ctx)
        , acceptor_(*ioc)
        , socket_(*ioc)
        , rpcServer(rpcServer)
    {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }
        
        // this solves the shut down problem
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        
        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        if(! acceptor_.is_open())
            return;
        do_accept();
    }

    void
    do_accept()
    {
        acceptor_.async_accept(
            socket_,
            std::bind(
                &listener::on_accept,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_accept(boost::system::error_code ec)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(std::move(socket_), *ctx_, rpcServer)->run();
        }

        // Accept another connection
        do_accept();
    }
};

std::shared_ptr<listener> listenerPtr;

namespace ketoEnv = keto::environment;

std::string RpcServer::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcServer::RpcServer() {
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
    
}

RpcServer::~RpcServer() {
}

RpcServerPtr RpcServer::init() {
    AccountSessionCache::init();
    return singleton = std::shared_ptr<RpcServer>(new RpcServer());
}

void RpcServer::fin() {
    singleton.reset();
    AccountSessionCache::fin();
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
    this->ioc = std::make_shared<boost::asio::io_context>(this->threads);

    // The SSL context is required, and holds certificates
    this->contextPtr = std::make_shared<beastSsl::context>(beastSsl::context::sslv23);
    
    // This holds the self-signed certificate used by the server
    load_server_certificate(*(this->contextPtr));
    
    // Create and launch a listening port
    listenerPtr = std::make_shared<listener>(ioc,
        contextPtr,
        tcp::endpoint{this->serverIp, this->serverPort},
        this);
    listenerPtr->run();
    
    // Run the I/O service on the requested number of threads
    this->threadsVector.reserve(this->threads);
    for(int i = 0; i < this->threads; i++) {
        this->threadsVector.emplace_back(
        [this]
        {
            this->ioc->run();
        });
    }
    KETO_LOG_INFO << "[RpcServer::start] All the threads have been started : " << this->threads;
}
    
void RpcServer::stop() {
    this->ioc->stop();
    
    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
            iter != this->threadsVector.end(); iter++) {
        iter->join();
    }
    
    listenerPtr.reset();
    
    this->threadsVector.clear();
}
    

void RpcServer::setSecret(
        const keto::crypto::SecureVector& secret) {
    this->secret = secret;
}


void RpcServer::setExternalIp(
        const boost::asio::ip::address& ipAddress) {
    if (this->externalIp.is_unspecified()) {
        this->externalIp = ipAddress;
        std::stringstream sstream;
        sstream << this->externalIp.to_string() << ":" << this->serverPort;
        RpcServerSession::getInstance()->addPeer(
                keto::server_common::ServerInfo::getInstance()->getAccountHash(),
                sstream.str());
    }
}


keto::crypto::SecureVector RpcServer::getSecret() {
    return this->secret;
}

keto::event::Event RpcServer::routeTransaction(const keto::event::Event& event) {
    keto::proto::MessageWrapper messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(messageWrapper);

    if (!AccountSessionCache::getInstance()->hasSession(
            messageWrapper.account_hash())) {
        std::stringstream ss;
        ss << "Account [" <<
                messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX)
                << "] is not bound as a peer.";
        BOOST_THROW_EXCEPTION(ClientNotAvailableException(
                ss.str()));
    }
    AccountSessionCache::getInstance()->getSession(
            messageWrapper.account_hash())->routeTransaction(messageWrapper);
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);

    std::stringstream ss;
    ss << "Routed to the peer [" <<
            messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
    response.set_result(ss.str());
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}


keto::event::Event RpcServer::pushBlock(const keto::event::Event& event) {
    for (std::string account: AccountSessionCache::getInstance()->getSessions()) {
        try {
            KETO_LOG_INFO << "[RpcServer::pushBlock] push block to node [" << Botan::hex_encode((const uint8_t*)account.c_str(),account.size(),true) << "]";
            sessionPtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                            account);
            if (sessionPtr_) {
                sessionPtr_->pushBlock(keto::server_common::fromEvent<keto::proto::SignedBlockWrapperMessage>(event));
            }
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
    for (std::string account: AccountSessionCache::getInstance()->getSessions()) {
        try {
            sessionPtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                    account);
            if (sessionPtr_) {
                sessionPtr_->performNetworkSessionReset();
            }
        }  catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performNetworkSessionReset]Failed to push the session reset : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::performNetworkSessionReset]Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performNetworkSessionReset]Failed to push the session reset : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer::performNetworkSessionReset]Failed to push the session reset : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::performNetworkSessionReset]Failed to push the session reset  : unknown cause";
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
    for (std::string account: AccountSessionCache::getInstance()->getSessions()) {
        try {
            sessionPtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                    account);
            if (sessionPtr_) {
                sessionPtr_->performProtocolCheck();
            }
        }  catch (keto::common::Exception& ex) {
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
    for (std::string account: AccountSessionCache::getInstance()->getSessions()) {
        try {
            KETO_LOG_INFO << "[RpcServer::performConsensusHeartbeat] perform consensus heart beat on [" << Botan::hex_encode((const uint8_t*)account.c_str(),account.size(),true) << "]";
            sessionPtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                    account);
            if (sessionPtr_) {
                sessionPtr_->performNetworkHeartbeat(
                        keto::server_common::fromEvent<keto::proto::ProtocolHeartbeatMessage>(event));
            }
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

}
}