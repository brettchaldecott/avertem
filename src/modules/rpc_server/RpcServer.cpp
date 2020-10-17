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
#include <queue>
#include <vector>

#include "SoftwareConsensus.pb.h"
#include "HandShake.pb.h"
#include "Route.pb.h"
#include "BlockChain.pb.h"
#include "Protocol.pb.h"
#include "KeyStore.pb.h"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <botan/hex.h>
#include <botan/base64.h>
#include <google/protobuf/message_lite.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/host_name.hpp>

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

#include "keto/software_consensus/ConsensusStateManager.hpp"

#include "keto/election_common/ElectionMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"
#include "keto/election_common/ElectionResultCache.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"
#include "keto/election_common/PublishedElectionInformationHelper.hpp"


#include "keto/rpc_protocol/NetworkStatusHelper.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {


class BufferCache {
public:
    BufferCache() {

    }
    BufferCache(const BufferCache& orig) = delete;
    virtual ~BufferCache() {
        for (boost::beast::flat_buffer* buffer : buffers) {
            delete buffer;
        }
        buffers.clear();
    }

    boost::beast::flat_buffer* create() {
        boost::beast::flat_buffer* buffer = new boost::beast::flat_buffer();
        this->buffers.insert(buffer);
        return buffer;
    }
    void remove(boost::beast::flat_buffer* buffer) {
        this->buffers.erase(buffer);
        delete buffer;
    }
private:
    std::set<boost::beast::flat_buffer*> buffers;
};
typedef std::shared_ptr<BufferCache> BufferCachePtr;

class BufferScope {
public:
    BufferScope(const BufferCachePtr& bufferCachePtr, boost::beast::flat_buffer* buffer) :
        bufferCachePtr(bufferCachePtr), buffer(buffer) {
    }
    BufferScope(const BufferScope& orig) = delete;
    virtual ~BufferScope() {
        bufferCachePtr->remove(buffer);
    }

private:
    BufferCachePtr bufferCachePtr;
    boost::beast::flat_buffer* buffer;
};


    static RpcServerPtr singleton;


class ReadQueueEntry {
public:
    ReadQueueEntry(const std::string& command, const std::string& payload) :
            command(command),payload(payload){}

    ReadQueueEntry(const ReadQueueEntry& orig) = delete;
    virtual ~ReadQueueEntry() {}


    std::string getCommand() {return this->command;}
    std::string getPayload() {return this->payload;}

private:
    std::string command;
    std::string payload;
};
typedef std::shared_ptr<ReadQueueEntry> ReadQueueEntryPtr;



class SessionBase {
public:
    virtual bool isActive() = 0;
    virtual void routeTransaction(keto::proto::MessageWrapper&  messageWrapper) = 0;
    virtual void pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) = 0;
    virtual void performNetworkSessionReset() = 0;
    virtual void performProtocolCheck() = 0;
    virtual void activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) = 0;
    virtual void performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage) = 0;
    virtual bool electBlockProducer() = 0;
    virtual void electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) = 0;
    virtual void electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) = 0;
    virtual void requestBlockSync(const keto::proto::SignedBlockBatchRequest& request) = 0;
    virtual void processQueueEntry(const ReadQueueEntryPtr& readQueueEntryPtr) = 0;
    virtual void closeSession() = 0;
};
typedef std::shared_ptr<SessionBase> SessionBasePtr;

class AccountSessionCache;
typedef std::shared_ptr<AccountSessionCache> AccountSessionCachePtr;

static AccountSessionCachePtr accountSessionSingleton;

class AccountSessionCache {
private:
    std::recursive_mutex classMutex;
    std::map<std::string,SessionBasePtr> accountSessionMap;

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
            const SessionBasePtr sessionRef) {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        accountSessionMap[account] = sessionRef;
    }
    
    void removeAccount(const std::string& account,const SessionBase* session) {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        if (!accountSessionMap.count(account)) {
            return;
        } /*else if (accountSessionMap[account].get() != session) {
            KETO_LOG_ERROR << "[AccountSessionCache][removeAccount] deactivating the status and notify";
            return;
        }*/

        accountSessionMap.erase(account);
    }
    
    bool hasSession(const std::string& account) {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        if (accountSessionMap.count(account)) {
            return true;
        }
        return false;
    }

    SessionBasePtr getSession(const std::string& account) {
        std::lock_guard<std::recursive_mutex> guard(this->classMutex);
        if (this->accountSessionMap.count(account)) {
            return this->accountSessionMap[account];
        }
        return SessionBasePtr();
    }

    std::vector<std::string> getSessions() {
        std::lock_guard<std::recursive_mutex> guard(classMutex);
        std::vector<std::string> keys;
        for(std::map<std::string,SessionBasePtr>::iterator it = this->accountSessionMap.begin();
            it != this->accountSessionMap.end(); ++it) {
            keys.push_back(it->first);
        }
        return keys;
    }

    std::vector<SessionBasePtr> getActiveSessions() {
        std::vector<SessionBasePtr> result;
        for(std::map<std::string,SessionBasePtr>::iterator it = this->accountSessionMap.begin();
            it != this->accountSessionMap.end(); ++it) {
            SessionBasePtr _sessionPtr = it->second;
            if (_sessionPtr->isActive()) {
                result.push_back(_sessionPtr);
            }
        }
        return result;
    }

    std::vector<SessionBasePtr> getSessionPtrs() {
        std::vector<SessionBasePtr> result;
        for(std::map<std::string,SessionBasePtr>::iterator it = this->accountSessionMap.begin();
            it != this->accountSessionMap.end(); ++it) {
            result.push_back(it->second);
        }
        return result;
    }
};


class ReadQueue {
public:
    ReadQueue(SessionBase* session) : session(session), active(true) {
        queueThreadPtr = std::shared_ptr<std::thread>(new std::thread(
                [this]
                {
                    this->run();
                }));
    }
    ReadQueue(const ReadQueue& orig) = delete;
    virtual ~ReadQueue() {
        deactivate();
    }

    void pushEntry(const std::string& command, const std::string& payload) {
        std::unique_lock<std::mutex> guard(classMutex);
        this->readQueue.push_back(ReadQueueEntryPtr(new ReadQueueEntry(command,payload)));
        stateCondition.notify_all();
    }

private:
    SessionBase* session;
    bool active;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    std::deque<ReadQueueEntryPtr> readQueue;
    std::shared_ptr<std::thread> queueThreadPtr;

    ReadQueueEntryPtr popEntry() {
        std::unique_lock<std::mutex> uniqueLock(classMutex);
        if (!this->readQueue.empty()) {
            ReadQueueEntryPtr result = this->readQueue.front();
            this->readQueue.pop_front();
            return result;
        }
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_SERVER_QUEUE_DELAY));
        return ReadQueueEntryPtr();
    }

    bool isActive() {
        std::unique_lock<std::mutex> guard(classMutex);
        return this->active;
    }

    void deactivate() {
        {
            std::unique_lock<std::mutex> guard(classMutex);
            KETO_LOG_ERROR << "[RpcServer][ReadQueue][deactivate] deactivating the status and notify";
            this->active = false;
            stateCondition.notify_all();
        }
        KETO_LOG_ERROR << "[RpcServer][ReadQueue][deactivate] wait for thread";
        queueThreadPtr->join();
        queueThreadPtr.reset();
        KETO_LOG_ERROR << "[RpcServer][ReadQueue][deactivate] finish";
    }

    void run() {
        KETO_LOG_ERROR << "[RpcServer][ReadQueue][run] beginning of run";
        while(this->isActive()) {
            ReadQueueEntryPtr entry = popEntry();
            if (entry) {
                this->session->processQueueEntry(entry);
            }
        }
        KETO_LOG_ERROR << "[RpcServer][ReadQueue][run] queue runner finished exiting";
    }



};

typedef std::shared_ptr<ReadQueue> ReadQueuePtr;


// Echoes back all received WebSocket messages
class session : public SessionBase, public std::enable_shared_from_this<session>
{
private:
    std::recursive_mutex classMutex;
    websocket::stream<
            beast::ssl_stream<beast::tcp_stream>> ws_;

    boost::asio::ip::address localAddress;
    std::string localHostname;
    std::string remoteAddress;
    std::string remoteHostname;

    //boost::beast::flat_buffer buffer_;
    boost::beast::flat_buffer sessionBuffer;
    RpcServer* rpcServer;
    std::shared_ptr<keto::rpc_protocol::ServerHelloProtoHelper> serverHelloProtoHelperPtr;
    keto::crypto::SecureVector sessionId;
    std::shared_ptr<Botan::AutoSeeded_RNG> generatorPtr;
    std::deque<std::shared_ptr<std::string>> queue_;
    ReadQueuePtr readQueuePtr;
    keto::election_common::ElectionResultCache electionResultCache;
    bool registered;
    bool active;
    bool closed;

public:
    // Take ownership of the socket
    session(tcp::socket&& socket, sslBeast::context& ctx, RpcServer* rpcServer)
        :
            ws_(std::move(socket), ctx)
        , rpcServer(rpcServer)
        , registered(false)
        , active(false)
        , closed(false)
    {
        ws_.auto_fragment(false);
        this->generatorPtr = std::shared_ptr<Botan::AutoSeeded_RNG>(new Botan::AutoSeeded_RNG());

        // setup the session read queue
        this->readQueuePtr = ReadQueuePtr(new ReadQueue((SessionBase*)this));

        // increment the session count
        RpcServer::getInstance()->incrementSessionCount();
    }
        
    virtual ~session() {
        KETO_LOG_ERROR << "[RpcServer][session] shutting down the session";
        //removeFromCache();
        this->readQueuePtr.reset();
        // increment the session count
        RpcServer::getInstance()->decrementSessionCount();
        KETO_LOG_ERROR << "[RpcServer][session] end of destructor";
    }

    keto::crypto::SecureVector getSessionId() {
        return this->sessionId;
    }

    bool isActive() {
        return this->active;
    }

    // Start the asynchronous operation
    void
    run()
    {
        // Set the timeout.
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Perform the SSL handshake
        ws_.next_layer().async_handshake(
                sslBeast::stream_base::server,
                beast::bind_front_handler(
                        &session::on_handshake,
                        shared_from_this()));
    }

    void
    on_handshake(boost::system::error_code ec)
    {
        if(ec) {
            close();
            return fail(ec, "handshake");
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
                websocket::stream_base::timeout::suggested(
                        beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
                [](websocket::response_type& res)
                {
                    res.set(http::field::server,
                            std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-server-async-ssl");
                }));

        // Accept the websocket handshake
        ws_.async_accept(
                beast::bind_front_handler(
                        &session::on_accept,
                        shared_from_this()));

    }

    void
    on_accept(boost::system::error_code ec)
    {
        if(ec) {
            close();
            return fail(ec, "accept");
        }


        try {
            boost::asio::io_service io_service;
            boost::asio::ip::tcp::resolver resolver(io_service);

            // local address
            this->localAddress = beast::get_lowest_layer(ws_).socket().local_endpoint().address();
            boost::asio::ip::tcp::resolver::iterator localIter = resolver.resolve(beast::get_lowest_layer(ws_).socket().local_endpoint());
            this->localHostname = localIter->host_name();


            // reverse lookup the hostname
            this->remoteAddress = beast::get_lowest_layer(ws_).socket().remote_endpoint().address().to_string();
            boost::asio::ip::tcp::resolver::iterator remoteIter = resolver.resolve(beast::get_lowest_layer(ws_).socket().remote_endpoint());
            this->remoteHostname = remoteIter->host_name();

        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][on_accept] Failed to handle the request [keto::common::Exception]: " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][on_accept] Failed to handle the request [boost::exception]: " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][on_accept] Failed to handle the request [std::exception]: " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer][on_accept] Failed to handle the request on the server [...]: unknown";
        }

        KETO_LOG_INFO << "[RpcServer][on_accept] Remote host information [" << this->remoteHostname << "]";
        KETO_LOG_INFO << "[RpcServer][on_accept] Local host information [" << this->localHostname << "]";
        // Read a message
        do_read();
    }

    void
    do_read()
    {
        if (RpcServer::getInstance()->isTerminated()) {
            close();
            return removeFromCache();
        }
        // Read a message into our buffer
        ws_.async_read(
                sessionBuffer,
                beast::bind_front_handler(
                        &session::on_read,
                        shared_from_this()));
    }

    void
    removeFromCache() {
        if (registered) {
            keto::router_utils::RpcPeerHelper rpcPeerHelper;
            rpcPeerHelper.setAccountHash(serverHelloProtoHelperPtr->getAccountHash());
            keto::server_common::triggerEvent(
                    keto::server_common::toEvent<keto::proto::RpcPeer>(
                            keto::server_common::Events::DEREGISTER_RPC_PEER,rpcPeerHelper));
        }
        if (serverHelloProtoHelperPtr) {
            std::string accountHash = keto::server_common::VectorUtils().copyVectorToString(
                    serverHelloProtoHelperPtr->getAccountHash());
            //KETO_LOG_DEBUG << "End of server session remove it from the account session cache : " <<
            //    Botan::hex_encode(serverHelloProtoHelperPtr->getAccountHash());
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
        std::size_t bytes_transferred) {
        keto::server_common::StringVector stringVector;

        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed || !ws_.is_open()) {
            close();
            return removeFromCache();
        }
        if (RpcServer::getInstance()->isTerminated()) {
            close();
            return removeFromCache();
        }
        if (ec) {
            return fail(ec, "read");
        }


        std::stringstream ss;
        ss << boost::beast::make_printable(sessionBuffer.data());
        std::string data = ss.str();
        stringVector = keto::server_common::StringUtils(data).tokenize(" ");


        // Clear the buffer
        sessionBuffer.consume(sessionBuffer.size());

        std::string command = stringVector[0];
        std::string payload;
        if (stringVector.size() == 2) {
            payload = stringVector[1];
        }

        this->readQueuePtr->pushEntry(command,payload);

        // do a read and wait for more messages
        do_read();
    }

    void processQueueEntry(const ReadQueueEntryPtr& readQueueEntryPtr) {
        std::string command = readQueueEntryPtr->getCommand();
        std::string payload = readQueueEntryPtr->getPayload();
        std::string message;
        
        try {
            //KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] process the command : " << command;
            keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
            if (keto::software_consensus::ConsensusStateManager::getInstance()->getState()
                != keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
                KETO_LOG_INFO << "[RpcServer] This peer is currently not configured reconnection required : "
                              << keto::software_consensus::ConsensusStateManager::getInstance()->getState();
                message = handleRetryResponse(command);
            } else {
                if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO) == 0) {
                    message = handleHello(command, payload);
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " Said hello";
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS) == 0) {
                    message = handleHelloConsensus(command, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PEERS) == 0) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " requested peers";
                    message = handlePeer(command, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REGISTER) == 0) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " register";
                    message = handleRegister(keto::server_common::Constants::RPC_COMMANDS::REGISTER, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS) == 0) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " push rpc peers";
                    handlePushRpcPeers(keto::server_common::Constants::RPC_COMMANDS::PUSH_RPC_PEERS, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE) == 0) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]" << this->serverHelloProtoHelperPtr->getAccountHashStr() << " activate";
                    handleActivate(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION) == 0) {
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] handle a transaction";
                    message = handleTransaction(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION, payload);
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Transaction processing complete";
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED) == 0) {
                    handleTransactionProcessed(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED,
                                               payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CONSENSUS) == 0) {

                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLOSE) == 0) {
                    // implement
                    //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] close the session";
                    return removeFromCache();
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS) ==
                           0) {
                    message = handleRequestNetworkSessionKeys(
                            keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS) ==
                           0) {
                    message = handleRequestMasterNetworkKeys(
                            keto::server_common::Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS) == 0) {
                    message = handleRequestNetworkKeys(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS,
                                                       payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES) == 0) {
                    message = handleRequestNetworkFees(keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES,
                                                       payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE) == 0) {
                    message = handleClientNetworkComplete(
                            keto::server_common::Constants::RPC_COMMANDS::CLIENT_NETWORK_COMPLETE, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK) == 0) {
                    handleBlockPush(keto::server_common::Constants::RPC_COMMANDS::BLOCK, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST) == 0) {
                    message = handleBlockSyncRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                                                     payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE) == 0) {
                    message = handleBlockSyncResponse(command, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE) == 0) {
                    message = handleProtocolCheckResponse(
                            keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_RESPONSE, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST) == 0) {
                    message = handleElectionRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                                                    payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE) == 0) {
                    handleElectionResponse(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH) == 0) {
                    handleElectionPublish(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH, payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION) == 0) {
                    handleElectionConfirmation(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                                               payload);
                } else if (command.compare(keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS) == 0) {
                    handleResponseNetworkStatus(command, payload);
                }
            }
            //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] processed the command : " << command;
            transactionPtr->commit();
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "] on the server [keto::common::Exception]: " << boost::diagnostic_information(ex,true);
            message = handleRetryResponse(command);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "]on the server [boost::exception]: " << boost::diagnostic_information(ex,true);
            message = handleRetryResponse(command);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "] on the server [std::exception]: " << ex.what();
            message = handleRetryResponse(command);
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer][processQueueEntry][" << getAccount() << "] Failed to handle the request [" << command << "] on the server [...]: unknown";
            message = handleRetryResponse(command);
        }

        // send a message
        //KETO_LOG_INFO << "[RpcServer] Finished processing : " << message;
        if (!message.empty()) {
            send(message);
        }

        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Finished processing.";
    }

    void
    activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Activate this node with its peer";
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        std::string rpcValue = rpcPeerHelper;
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::ACTIVATE, Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true));
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] Activate this node with its peer";
    }

    void
    electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerPublish]: publish the election result";
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
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_PUBLISH,
                      Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerPublish]: published the election result";
    }

    void
    electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerConfirmation]: confirmation of the election results";
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

        clientRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_CONFIRMATION,
                      Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "[session::electBlockProducerConfirmation]: confirmation sent for election results";
    }

    void requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        std::string messageWrapperStr = signedBlockBatchRequest.SerializeAsString();
        std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
                messageWrapperStr);

        KETO_LOG_INFO << "[session::requestBlockSync][" << getAccount() << "] request the block sync from a";
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST,
                      Botan::hex_encode((uint8_t*)messageBytes.data(),messageBytes.size(),true));
        //KETO_LOG_DEBUG << "[session::requestBlockSync][" << getAccount() << "] The request for [" <<
        //              keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_REQUEST << "]";
    }

    void
    routeTransaction(keto::proto::MessageWrapper&  messageWrapper) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]Route transaction";
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        std::string messageWrapperStr;
        messageWrapper.SerializeToString(&messageWrapperStr);
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::TRANSACTION,
                      Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]Routed the transaction";
    }

    void
    pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]Attempt to push the block";
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        std::string messageWrapperStr;
        signedBlockWrapperMessage.SerializeToString(&messageWrapperStr);
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::BLOCK,
                      Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]After writing the block";
    }

    //
    //
    // the protocol methods
    void
    performNetworkSessionReset() {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]Attempt to perform the protocol reset via forcing the client to say hello";
        // different message format
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS
                                               << " " << Botan::hex_encode(this->rpcServer->getSecret())
                                               << " " << Botan::hex_encode(this->generateSession());
        send(ss.str());
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]After requesting the protocol reset from peers";
    }


    void
    performProtocolCheck() {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]Attempt to perform the protocol check";
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_REQUEST,
                      Botan::hex_encode(this->generateSession()));

        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "]After requesting the protocol check from peers";
    }



    void
    performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage) {
        //KETO_LOG_DEBUG << "Attempt to perform the protocol heartbeat";
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return;
        }
        this->electionResultCache.heartBeat(protocolHeartbeatMessage);
        std::string messageWrapperStr;
        protocolHeartbeatMessage.SerializeToString(&messageWrapperStr);
        clientRequest(keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_HEARTBEAT,
                Botan::hex_encode((uint8_t*)messageWrapperStr.data(),messageWrapperStr.size(),true));
        //KETO_LOG_DEBUG << "After requesting the protocol heartbeat";
    }

    bool
    electBlockProducer() {
        //KETO_LOG_DEBUG << getAccount() << "[electBlockProducer]: elect block producer";
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        if (this->closed) {
            return false;
        }
        if (this->isActive()) {
            return false;
        }
        keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper;
        electionPeerMessageProtoHelper.setAccount(keto::server_common::ServerInfo::getInstance()->getAccountHash());

        std::vector<uint8_t> messageBytes =  keto::server_common::VectorUtils().copyStringToVector(
                electionPeerMessageProtoHelper);

        clientRequest(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_REQUEST,
                           messageBytes);
        return true;
        //KETO_LOG_DEBUG << getAccount() << "[electBlockProducer]: after invoking election ";
    }

    std::string
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
        std::stringstream ss;
        if (moduleConsensusValidationMessageHelper.isValid()) {
            ss << keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT
                                           << " " << keto::server_common::Constants::RPC_COMMANDS::PROTOCOL_CHECK_ACCEPT;
            //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was accepted";
        } else {
            ss << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY
                                           << " " << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY;
            //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was rejected from network";
        }
        return ss.str();
    }

    std::string
    handleElectionRequest(const std::string& command, const std::string& payload) {
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
        //KETO_LOG_DEBUG << "[RpcServer::handleElectionRequest] elected a node : " << command;
        return buildMessage(keto::server_common::Constants::RPC_COMMANDS::ELECT_NODE_RESPONSE,
                      Botan::hex_encode((uint8_t*)result.data(),result.size(),true));
    }

    void
    handleElectionResponse(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer::handleElectionResponse] handle the election response : " << command;
        keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper(
                keto::server_common::VectorUtils().copyVectorToString(
                        Botan::hex_decode(payload)));

        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::ElectionResultMessage>(
                        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE,electionResultMessageProtoHelper));
        //KETO_LOG_DEBUG << "[RpcServer::handleElectionResponse] handled the election response : " << command;
    }

    void handleElectionPublish(const std::string& command, const std::string& message) {
        //KETO_LOG_DEBUG << "[handleElectionPublish] handle the election";
        keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
                keto::server_common::VectorUtils().copyVectorToString(
                        Botan::hex_decode(message)));

        // prevent echo propergation at the boundary
        if (this->electionResultCache.containsPublishAccount(electionPublishTangleAccountProtoHelper.getAccount())) {
            //KETO_LOG_DEBUG << "[handleElectionPublish] handle the election";
            return;
        }

        //KETO_LOG_DEBUG << "[handleElectionPublish] publish the election";
        keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_PUBLISH).
                publish(electionPublishTangleAccountProtoHelper);
    }


    void handleElectionConfirmation(const std::string& command, const std::string& message) {
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


    void clientRequest(const std::string& command, const std::vector<uint8_t>& message) {
        //KETO_LOG_DEBUG << "[RpcServer] Send the server request : " << command;
        clientRequest(command,Botan::hex_encode((uint8_t*)message.data(),message.size(),true));
        //KETO_LOG_DEBUG << "[RpcServer] Sent the server request : " << command;
    }

    void clientRequest(const std::string& command, const std::string& message) {
        //KETO_LOG_DEBUG << "[RpcServer] Send the server request : " << command;
        send(buildMessage(command,message));
        //KETO_LOG_DEBUG << "[RpcServer] Sent the server request : " << command;
    }


    void closeSession() {
        close();
        send(keto::server_common::Constants::RPC_COMMANDS::CLOSE);
    }


    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        queue_.pop_front();

        if(ec)
            return fail(ec, "write");

        if (queue_.size()) {
            sendFirstQueueMessage();
        }
    }
    
    std::string buildMessage(const std::string& command, const std::string& message) {
        std::stringstream ss;
        ss << command << " " << message;
        return ss.str();
    }
    
    std::string handleHello(const std::string& command, const std::string& payload) {

        std::string bytes = keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload));
        this->serverHelloProtoHelperPtr = 
                std::shared_ptr<keto::rpc_protocol::ServerHelloProtoHelper>(
                new keto::rpc_protocol::ServerHelloProtoHelper(bytes));
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::HELLO_CONSENSUS
                << " " << Botan::hex_encode(this->rpcServer->getSecret()) 
                << " " << Botan::hex_encode(
                this->generateSession());
        return ss.str();
    }
    
    std::string handleHelloConsensus(const std::string& command, const std::string& payload) {
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
            ss << keto::server_common::Constants::RPC_COMMANDS::ACCEPTED
                << " " << keto::server_common::Constants::RPC_COMMANDS::ACCEPTED;
            //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was accepted";
        } else {
            ss << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY
                << " " << keto::server_common::Constants::RPC_COMMANDS::GO_AWAY;
            //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " was rejected from network";
        }
        return ss.str();
    }
    
    std::string handlePeer(const std::string& command, const std::string& payload) {
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
        this->rpcServer->setExternalIp(this->localAddress);
        this->rpcServer->setExternalHostname(this->localHostname);
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
            peers.push_back(this->rpcServer->getExternalPeerInfo());
        }
        peerResponseHelper.addPeers(peers);

        // return the list of peers
        std::string result;
        peerResponseHelper.operator keto::proto::PeerResponse().SerializePartialToString(&result);
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::PEERS
                << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        //KETO_LOG_DEBUG << "[RpcServer] " << this->serverHelloProtoHelperPtr->getAccountHashStr() << " return the peers to the client";
        return ss.str();
    }
    
    std::string handleRegister(const std::string& command, const std::string& payload) {

        // add this session for call backs
        AccountSessionCache::getInstance()->addAccount(
            keto::server_common::VectorUtils().copyVectorToString(    
            serverHelloProtoHelperPtr->getAccountHash()),
                shared_from_this());

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
        this->rpcServer->setExternalIp(this->localAddress);
        this->rpcServer->setExternalHostname(this->localHostname);
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
        this->active = rpcPeerHelper.isActive();

        // set up the peer information
        keto::router_utils::RpcPeerHelper serverPeerHelper;
        serverPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
        serverPeerHelper.setActive(RpcServer::getInstance()->isServerActive());
        std::string serverPeerStr = serverPeerHelper;

        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::REGISTER
                << " " << Botan::hex_encode((uint8_t*)serverPeerStr.data(),serverPeerStr.size(),true);
        return ss.str();
    }


    void handlePushRpcPeers(const std::string& command, const std::string& payload) {
        std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload));
        keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);


        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                        keto::server_common::Events::ROUTER_QUERY::PROCESS_PUSH_RPC_PEER,rpcPeerHelper));

    }


    void handleActivate(const std::string& command, const std::string& payload) {
        std::string rpcVector = keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload));
        keto::router_utils::RpcPeerHelper rpcPeerHelper(rpcVector);
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleActivate] activate the node : " << rpcPeerHelper.isActive();
        this->active = rpcPeerHelper.isActive();

        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::RpcPeer>(
                        keto::server_common::Events::ACTIVATE_RPC_PEER,rpcPeerHelper));
    }
    
    
    std::string handleTransaction(const std::string& command, const std::string& payload) {
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
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::TRANSACTION_PROCESSED
                << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);

        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleTransaction] process the transaction";
        return ss.str();
    }
    
    void handleTransactionProcessed(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleTransactionProcessed] handle the transaction";

        keto::proto::MessageWrapperResponse messageWrapperResponse;
        messageWrapperResponse.ParseFromString(
            keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload)));

        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "] transaction processed by peer [" <<
        //        this->serverHelloProtoHelperPtr->getAccountHashStr() << "] : "
        //        << messageWrapperResponse.result();
        
    }

    std::string handleRequestNetworkSessionKeys(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkSessionKeys] << request the network session keys :" << command;

        keto::proto::NetworkKeysWrapper networkKeysWrapper;
        networkKeysWrapper =
                keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                                keto::server_common::Events::GET_NETWORK_SESSION_KEYS,networkKeysWrapper)));

        std::string result = networkKeysWrapper.SerializeAsString();
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] set the response for the network session keys";
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkSessionKeys] << after handling the network session keys request :" << command;
        return ss.str();
    }

    std::string handleRequestMasterNetworkKeys(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestMasterNetworkKeys] << request the master network keys :" << command;

        keto::proto::NetworkKeysWrapper networkKeysWrapper;
        networkKeysWrapper =
                keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                                keto::server_common::Events::GET_MASTER_NETWORK_KEYS,networkKeysWrapper)));

        std::string result = networkKeysWrapper.SerializeAsString();
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] set the response for the network keys";
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestMasterNetworkKeys] << after handling the master network keys :" << command;
        return ss.str();
    }

    std::string handleRequestNetworkKeys(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] << request the network key :" << command;

        keto::proto::NetworkKeysWrapper networkKeysWrapper;
        networkKeysWrapper =
                keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(
                                keto::server_common::Events::GET_NETWORK_KEYS,networkKeysWrapper)));

        std::string result = networkKeysWrapper.SerializeAsString();
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] send the response";
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);

        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkKeys] << after handling the request:" << command;
        return ss.str();
    }


     std::string handleRequestNetworkFees(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkFees] handle request network fees";
        keto::proto::FeeInfoMsg feeInfoMsg;
        feeInfoMsg =
                keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::FeeInfoMsg>(
                                keto::server_common::Events::NETWORK_FEE_INFO::GET_NETWORK_FEE,feeInfoMsg)));

        std::string result = feeInfoMsg.SerializeAsString();
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkFees] setup the network fees response";
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleRequestNetworkFees] request network fees";
        return ss.str();
    }

    std::string handleClientNetworkComplete(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleClientNetworkComplete] The client has completed networking";
        std::stringstream ss;
        // At present the network status is not required
        if (!RpcServer::getInstance()->hasNetworkState()) {
            //KETO_LOG_ERROR << "[RpcServer][" << getAccount() << "][handleClientNetworkComplete] request network status";
            ss << keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS
               << " " << keto::server_common::Constants::RPC_COMMANDS::REQUEST_NETWORK_STATUS;
        } else {
            // respond with the published election information
            keto::proto::PublishedElectionInformation publishedElectionInformation;
            publishedElectionInformation =
                    keto::server_common::fromEvent<keto::proto::PublishedElectionInformation>(
                            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
                                    keto::server_common::Events::ROUTER_QUERY::GET_PUBLISHED_ELECTION_INFO,publishedElectionInformation)));

            std::string result = publishedElectionInformation.SerializeAsString();
            //KETO_LOG_DEBUG << "[RpcSession][handleRequestNetworkStatus] retrieve the network status";
            ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_NETWORK_STATUS
               << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
            //KETO_LOG_DEBUG << "[RpcSession][handleRequestNetworkStatus] provide the network status";
        }
        // at present there is no requirement to activate as this is handled in the registration response.
        //if (RpcServer::getInstance()->isServerActive()) {
        //    keto::router_utils::RpcPeerHelper rpcPeerHelper;
        //    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
        //    rpcPeerHelper.setActive(RpcServer::getInstance()->isServerActive());
        //    std::string rpcValue = rpcPeerHelper;
        //    ss << keto::server_common::Constants::RPC_COMMANDS::ACTIVATE << " "
        //        << Botan::hex_encode((uint8_t*)rpcValue.data(),rpcValue.size(),true);
        //}
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleClientNetworkComplete] Finished post network configuration";
        return ss.str();
    }

    std::string handleResponseNetworkStatus(const std::string& command, const std::string& payload) {
        keto::election_common::PublishedElectionInformationHelper publishedElectionInformationHelper(
                keto::server_common::VectorUtils().copyVectorToString(
                        Botan::hex_decode(payload)));

        KETO_LOG_ERROR << "[RpcServer][" << getAccount() << "][handleResponseNetworkStatus] set the network status";
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::PublishedElectionInformation>(
                keto::server_common::Events::ROUTER_QUERY::SET_PUBLISHED_ELECTION_INFO,publishedElectionInformationHelper));
        RpcServer::getInstance()->enableNetworkState();
        KETO_LOG_ERROR << "[RpcServer][" << getAccount() << "][handleResponseNetworkStatus] setup network status";

        return std::string();
    }

    void handleBlockPush(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockPush] handle block push";
        keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
        signedBlockWrapperMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload)));

        keto::proto::MessageWrapperResponse messageWrapperResponse =
                keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
                                keto::server_common::Events::BLOCK_PERSIST_MESSAGE,signedBlockWrapperMessage)));
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockPush] pushed the block";

    }

    std::string handleBlockSyncRequest(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] handle the block sync request : " << command;
        if (!RpcServer::getInstance()->isServerActive()) {
            //KETO_LOG_INFO << "[RpcServer] This server is inactive hand has not synced cannot handle this request.";
            return handleRetryResponse(command);
        }
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
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_RESPONSE
                                       << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncRequest] The block data returned [" << result.size() << "]";
        return ss.str();
    }

    std::string handleBlockSyncResponse(const std::string& command, const std::string& payload) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncResponse] handle the block sync request : " << command;
        keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
        signedBlockBatchMessage.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(
                Botan::hex_decode(payload)));

        keto::proto::MessageWrapperResponse messageWrapperResponse =
                keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
                                keto::server_common::Events::BLOCK_DB_RESPONSE_BLOCK_SYNC,signedBlockBatchMessage)));

        std::string result = messageWrapperResponse.SerializeAsString();
        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::BLOCK_SYNC_PROCESSED
           << " " << Botan::hex_encode((uint8_t*)result.data(),result.size(),true);
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][handleBlockSyncResponse] block sync response processed : " << command;
        return ss.str();
    }

    std::string handleRetryResponse(const std::string& command) {

        std::stringstream ss;
        ss << keto::server_common::Constants::RPC_COMMANDS::RESPONSE_RETRY
                                       << " " << command;
        return ss.str();
    }

    keto::crypto::SecureVector generateSession() {
        return this->sessionId = this->generatorPtr->random_vec(Constants::SESSION_ID_LENGTH);
    }

    void
    send(const std::string& message) {
        if (!ws_.is_open()) {
            return;
        }
        net::post(
            ws_.get_executor(),
            beast::bind_front_handler(
                &session::sendMessage,
                shared_from_this(),
                std::make_shared<std::string>(message)));
    }


    void
    sendMessage(std::shared_ptr<std::string> ss) {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][sendMessage] : push an entry into the queue";

        // Always add to queue
        queue_.push_back(ss);

        // Are we already writing?
        if (queue_.size() > 1) {
            return;
        }

        sendFirstQueueMessage();
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][sendMessage] : send a message";
    }

    void
    on_close(boost::system::error_code ec) {
        // closed the connection
        if(ec)
            return fail(ec, "close");
        KETO_LOG_INFO << "[RpcServer][" << getAccount() << "] Closed the connection";
    }

    void
    close() {
        std::unique_lock<std::recursive_mutex> uniqueLock(classMutex);
        this->closed = true;
    }

    void sendFirstQueueMessage() {
        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][sendFirstQueueMessage] send the message from the message buffer : " <<
        //    keto::server_common::StringUtils(*queue_.front()).tokenize(" ")[0];
        // We are not currently writing, so send this immediately

        // Echo the message
        std::string message = *queue_.front();
        if (!ws_.is_open()) {
            // is not open cannot send
            return;
        } else if (message == keto::server_common::Constants::RPC_COMMANDS::CLOSE) {
            ws_.async_close(websocket::close_code::normal,
                            beast::bind_front_handler(
                                    &session::on_close,
                                    shared_from_this()));
        } else {
            ws_.text(ws_.got_text());
            ws_.async_write(
                    boost::asio::buffer(*queue_.front()),
                    beast::bind_front_handler(
                            &session::on_write,
                            shared_from_this()));
        }


        //KETO_LOG_DEBUG << "[RpcServer][" << getAccount() << "][sendFirstQueueMessage] after sending the message";
    }


private:
    // Report a failure
    void
    fail(boost::system::error_code ec, char const* what)
    {
        removeFromCache();
        KETO_LOG_ERROR << "Failed to process because : " << what << ": " << ec.message();
    }

};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    std::shared_ptr<boost::asio::io_context> ioc_;
    std::shared_ptr<sslBeast::context> ctx_;
    tcp::acceptor acceptor_;
    RpcServer* rpcServer;

public:
    listener(
        std::shared_ptr<boost::asio::io_context> ioc,
        std::shared_ptr<sslBeast::context> ctx,
        tcp::endpoint endpoint,
        RpcServer* rpcServer)
        : ioc_(ioc)
        , ctx_(ctx)
        , acceptor_(net::make_strand(*ioc))
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
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }
        
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
        // The new connection gets its own strand
        acceptor_.async_accept(
                net::make_strand(*ioc_),
                beast::bind_front_handler(
                        &listener::on_accept,
                        shared_from_this()));
    }

    void
    on_accept(boost::system::error_code ec, tcp::socket socket)
    {
        if(ec) {
            return fail(ec, "accept");
        } else {
            // Create the session and run it
            std::make_shared<session>(std::move(socket), *ctx_, rpcServer)->run();
        }

        // Accept another connection
        do_accept();
    }

    // Report a failure
    void
    fail(boost::system::error_code ec, char const* what)
    {
        KETO_LOG_ERROR << "Failed to process because : " << what << ": " << ec.message();
    }

    void
    terminate() {
        acceptor_.cancel();
    }

};

std::shared_ptr<listener> listenerPtr;

namespace ketoEnv = keto::environment;

std::string RpcServer::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcServer::RpcServer() : externalHostname(""), sessionCount(0), serverActive(false), networkState(true), terminated(false) {
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


    // setup the external hostname information
    if (config->getVariablesMap().count(Constants::EXTERNAL_HOSTNAME)) {
        this->externalHostname =
                config->getVariablesMap()[Constants::EXTERNAL_HOSTNAME].as<std::string>();
    } else {
        this->externalHostname = boost::asio::ip::host_name();
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
    this->ioc = std::make_shared<net::io_context>(this->threads);

    // The SSL context is required, and holds certificates
    this->contextPtr = std::make_shared<sslBeast::context>(sslBeast::context::tlsv12);
    
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
    //KETO_LOG_DEBUG << "[RpcServer::start] All the threads have been started : " << this->threads;
}

void RpcServer::preStop() {

    KETO_LOG_ERROR << "[RpcServer] begin pre stop";
    this->terminate();
    listenerPtr->terminate();

    KETO_LOG_ERROR << "[RpcServer] wait for session end";
    this->waitForSessionEnd();

    KETO_LOG_ERROR << "[RpcServer] ioc stop";
    this->ioc->stop();

    KETO_LOG_ERROR << "[RpcServer] terminate threads";
    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
         iter != this->threadsVector.end(); iter++) {
        iter->join();
    }
    this->threadsVector.clear();

    KETO_LOG_ERROR << "[RpcServer] clear";
    listenerPtr.reset();

    KETO_LOG_ERROR << "[RpcServer] end pre stop";
}

void RpcServer::stop() {

}
    

void RpcServer::setSecret(
        const keto::crypto::SecureVector& secret) {
    this->secret = secret;
}


void RpcServer::setExternalIp(
        const boost::asio::ip::address& ipAddress) {
    if (this->externalIp.is_unspecified()) {
        this->externalIp = ipAddress;
        //std::stringstream sstream;
        //sstream << this->externalIp.to_string() << ":" << this->serverPort;
        //RpcServerSession::getInstance()->addPeer(
        //        keto::server_common::ServerInfo::getInstance()->getAccountHash(),
        //        sstream.str());
    }
}


void RpcServer::setExternalHostname(
        const std::string& externalHostname) {
    if (this->externalHostname.empty()) {
        this->externalHostname = externalHostname;
    }
}

bool RpcServer::hasNetworkState() {
    return this->networkState;
}


void RpcServer::enableNetworkState() {
    keto::proto::RequestNetworkState requestNetworkState;
    // these method are currently not completely implemented but are there as a means to sync with the network state
    // should this later be required.
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RequestNetworkState>(
            keto::server_common::Events::ACTIVATE_NETWORK_STATE_CLIENT,requestNetworkState));
    this->networkState = true;
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
            //KETO_LOG_DEBUG << "[RpcServer::pushBlock] push block to node [" << Botan::hex_encode((const uint8_t*)account.c_str(),account.size(),true) << "]";
            SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
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
            SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
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
            SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
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
            //KETO_LOG_DEBUG << "[RpcServer::performConsensusHeartbeat] perform consensus heart beat on [" << Botan::hex_encode((const uint8_t*)account.c_str(),account.size(),true) << "]";
            SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
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

keto::event::Event RpcServer::electBlockProducer(const keto::event::Event& event) {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());

    keto::election_common::ElectionMessageProtoHelper electionMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionMessage>(event));

    std::vector<std::string> accounts = AccountSessionCache::getInstance()->getSessions();
    for (int index = 0; (index < keto::server_common::Constants::ELECTION::ELECTOR_COUNT) && (accounts.size()); index++) {

        std::string account;
        if (accounts.size() > 1) {
            std::uniform_int_distribution<int> distribution(0, accounts.size() - 1);
            distribution(stdGenerator);
            int pos = distribution(stdGenerator);
            account = accounts[pos];
            accounts.erase(accounts.begin() + pos);
        } else {
            account = accounts[0];
            accounts.clear();
        }

        SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                account);
        //KETO_LOG_DEBUG << "[RpcServer::electBlockProducer] elect an account ["
        //    << keto::asn1::HashHelper(account).getHash(keto::common::StringEncoding::HEX) << "][" << sessionPtr_->isActive() << "]";
        if (sessionPtr_) {
            try {
                if (sessionPtr_->electBlockProducer()) {
                    electionMessageProtoHelper.addAccount(keto::asn1::HashHelper(account));
                }
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcServer::electBlockProducer] Failed call the election : " << ex.what();
                KETO_LOG_ERROR << "[RpcServer::electBlockProducer] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcServer::electBlockProducer] Failed call the election : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcServer::electBlockProducer] Failed call the election : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcServer::electBlockProducer] Failed call the election : unknown cause";
            }
        }
    }

    return keto::server_common::toEvent<keto::proto::ElectionMessage>(electionMessageProtoHelper);
}


keto::event::Event RpcServer::activatePeers(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    serverActive = rpcPeerHelper.isActive();
    std::vector<std::string> peers = AccountSessionCache::getInstance()->getSessions();
    for (std::string peer : peers)
    {
        SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                peer);
        if (sessionPtr_) {
            try {
                sessionPtr_->activatePeer(rpcPeerHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::activatePeer] Failed to activate the peer : unknown cause";
            }
        }
    }
    return event;
}

keto::event::Event RpcServer::requestNetworkState(const keto::event::Event& event) {
    KETO_LOG_INFO << "Request network state from client";
    this->networkState = false;
    return event;
}

keto::event::Event RpcServer::activateNetworkState(const keto::event::Event& event) {
    if (!this->networkState) {
        KETO_LOG_INFO << "Activte network state from client";
        this->networkState = true;
    }
    return event;
}

keto::event::Event RpcServer::requestBlockSync(const keto::event::Event& event) {
    keto::proto::SignedBlockBatchRequest request = keto::server_common::fromEvent<keto::proto::SignedBlockBatchRequest>(event);
    std::vector<SessionBasePtr> sessionBasePtrVector = AccountSessionCache::getInstance()->getActiveSessions();
    if (sessionBasePtrVector.size() == 0) {
        sessionBasePtrVector = AccountSessionCache::getInstance()->getSessionPtrs();
    }
    if (sessionBasePtrVector.size()) {
        // select a random client if the session base list is greater then 1
        int pos = 0;
        if (sessionBasePtrVector.size()>1) {
            std::default_random_engine stdGenerator;
            stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<int> distribution(0, sessionBasePtrVector.size() - 1);
            // seed
            distribution(stdGenerator);

            // retrieve
            pos = distribution(stdGenerator);
        }
        // request the block sync from up stream
        try {
            sessionBasePtrVector[pos]->requestBlockSync(request);
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : " << ex.what();
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Cause : " << boost::diagnostic_information(ex, true);
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : "
                           << boost::diagnostic_information(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Failed to request a block sync : unknown cause";
        }
    } else {
        // this will force a call to the RPC server to sync
        KETO_LOG_ERROR << "[RpcServer::requestBlockSync] Fall back failed no downstream active nodes";
    }

    return event;
}

bool RpcServer::isServerActive() {
    return (this->serverActive || keto::server_common::ServerInfo::getInstance()->isMaster());
}

std::string RpcServer::getExternalPeerInfo() {
    std::stringstream sstream;
    std::string hostname = this->externalIp.to_string();
    if (!externalHostname.empty()) {
        hostname = externalHostname;
    }

    sstream << hostname << ":" << this->serverPort;
    return sstream.str();
}

bool RpcServer::isTerminated() {
    std::lock_guard<std::mutex> guard(classMutex);
    return terminated;
}

void RpcServer::terminate() {
    std::lock_guard<std::mutex> guard(classMutex);
    this->terminated = true;
}


int RpcServer::getSessionCount() {
    std::unique_lock<std::mutex> unique_lock(classMutex);
    return this->sessionCount;
}

// session count
int RpcServer::incrementSessionCount() {
    std::unique_lock<std::mutex> unique_lock(classMutex);
    int result = ++this->sessionCount;
    stateCondition.notify_all();
    return result;
}

int RpcServer::decrementSessionCount() {
    std::unique_lock<std::mutex> unique_lock(classMutex);
    int result = --this->sessionCount;
    stateCondition.notify_all();
    return result;
}

void RpcServer::waitForSessionEnd() {
    std::unique_lock<std::mutex> unique_lock(classMutex);
    KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] waitForSessionEnd : " << this->sessionCount;
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point runTime = startTime;
    while(this->sessionCount)  {
        // get the list of sessions
        if (runTime == startTime ||
            (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - runTime).count() > 1500)) {
            for (std::string account: AccountSessionCache::getInstance()->getSessions()) {
                try {
                    //KETO_LOG_DEBUG << "[RpcServer::pushBlock] push block to node [" << Botan::hex_encode((const uint8_t*)account.c_str(),account.size(),true) << "]";
                    SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(
                            account);
                    if (sessionPtr_) {
                        sessionPtr_->closeSession();
                    }
                } catch (keto::common::Exception &ex) {
                    KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << ex.what();
                    KETO_LOG_ERROR << "[RpcServer::pushBlock]Cause : " << boost::diagnostic_information(ex, true);
                } catch (boost::exception &ex) {
                    KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : "
                                   << boost::diagnostic_information(ex, true);
                } catch (std::exception &ex) {
                    KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : " << ex.what();
                } catch (...) {
                    KETO_LOG_ERROR << "[RpcServer::pushBlock]Failed to push the block : unknown cause";
                }
            }
        }

        this->stateCondition.wait_for(unique_lock,std::chrono::milliseconds(1000));
        KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] waitForSessionEnd : " << this->sessionCount;
    }
}

keto::event::Event RpcServer::electBlockProducerPublish(const keto::event::Event& event) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));

    std::vector<std::string> peers = AccountSessionCache::getInstance()->getSessions();
    for (std::string peer : peers) {

        // get the account
        SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(peer);
        if (sessionPtr_) {
            try {
                sessionPtr_->electBlockProducerPublish(electionPublishTangleAccountProtoHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle change: " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle changes : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle changes : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerPublish] Failed to publish the tangle changes : unknown cause";
            }
        }
    }

    return event;
}


keto::event::Event RpcServer::electBlockProducerConfirmation(const keto::event::Event& event) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));
    std::vector<std::string> peers = AccountSessionCache::getInstance()->getSessions();
    for (std::string peer : peers) {

        // get the account
        SessionBasePtr sessionPtr_ = AccountSessionCache::getInstance()->getSession(peer);
        if (sessionPtr_) {
            try {
                sessionPtr_->electBlockProducerConfirmation(electionConfirmationHelper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle change: " << ex.what();
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle changes : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle changes : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcSessionManager::electBlockProducerConfirmation] Failed to publish the tangle changes : unknown cause";
            }
        }
    }

    return event;
}


}
}
