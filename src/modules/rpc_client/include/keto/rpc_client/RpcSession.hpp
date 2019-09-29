/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcSession.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 12:32 PM
 */

#ifndef RPCSESSION_HPP
#define RPCSESSION_HPP

#include <memory>
#include <queue>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <set>
#include <thread>
#include <condition_variable>

#include "Route.pb.h"
#include "BlockChain.pb.h"
#include "Protocol.pb.h"

#include "keto/event/Event.hpp"


#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/RpcPeer.hpp"
#include "keto/crypto/KeyLoader.hpp"

#include "keto/asn1/HashHelper.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"
#include "keto/election_common/ElectionResultCache.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/common/MetaInfo.hpp"

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace boostSsl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

namespace keto {
namespace rpc_client {

class RpcSession;

typedef std::shared_ptr<RpcSession> RpcSessionPtr;
typedef std::shared_ptr<boost::beast::multi_buffer> MultiBufferPtr;
typedef std::shared_ptr<std::lock_guard<std::mutex>> LockGuardPtr;

class RpcSession : public std::enable_shared_from_this<RpcSession> {
public:
    class BufferCache {
    public:
        BufferCache();
        BufferCache(const BufferCache& orig) = delete;
        virtual ~BufferCache();

        boost::beast::multi_buffer* create();
        void remove(boost::beast::multi_buffer* buffer);
    private:
        std::set<boost::beast::multi_buffer*> buffers;
    };
    typedef std::shared_ptr<BufferCache> BufferCachePtr;
    class BufferScope {
    public:
        BufferScope(const BufferCachePtr& bufferCachePtr, boost::beast::multi_buffer* buffer);
        BufferScope(const BufferScope& orig) = delete;
        virtual ~BufferScope();

    private:
        BufferCachePtr bufferCachePtr;
        boost::beast::multi_buffer* buffer;
    };

    class ReadQueueEntry {
    public:
        ReadQueueEntry(const std::string& command, const keto::server_common::StringVector& stringVector) :
            command(command),stringVector(stringVector){}

        ReadQueueEntry(const ReadQueueEntry& orig) = delete;
        virtual ~ReadQueueEntry() {}


        std::string getCommand() {return this->command;}
        keto::server_common::StringVector getStringVector() {return this->stringVector;}

    private:
        std::string command;
        keto::server_common::StringVector stringVector;
    };
    typedef std::shared_ptr<ReadQueueEntry> ReadQueueEntryPtr;

    class ReadQueue {
    public:
        ReadQueue(RpcSession* session);
        ReadQueue(const ReadQueue& orig) = delete;
        virtual ~ReadQueue();

        bool isActive();
        void activate();
        void deactivate();
        void pushEntry(const std::string& command, const keto::server_common::StringVector& stringVector);

    private:
        RpcSession* session;
        bool active;
        std::mutex classMutex;
        std::condition_variable stateCondition;
        std::deque<ReadQueueEntryPtr> readQueue;
        std::shared_ptr<std::thread> queueThreadPtr;

        ReadQueueEntryPtr popEntry();
        void run();
    };
    typedef std::shared_ptr<ReadQueue> ReadQueuePtr;


    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    RpcSession(
            std::shared_ptr<boost::asio::io_context> ioc, 
            std::shared_ptr<boostSsl::context> ctx,
            const RpcPeer& rpcPeer);
    RpcSession(const RpcSession& orig) = delete;
    virtual ~RpcSession();
    
    void run();
    void on_resolve(
        boost::system::error_code ec,
        tcp::resolver::results_type results);
    void
    on_connect(boost::system::error_code ec);
    
    void
    on_ssl_handshake(boost::system::error_code ec);
    
    void
    on_handshake(boost::system::error_code ec);

    
    void
    on_write(boost::system::error_code ec,
            std::size_t bytes_transferred);

    void
    do_read();

    void
    on_read(
            boost::system::error_code ec,
            std::size_t bytes_transferred);

    void processQueueEntry(const ReadQueueEntryPtr& readQueueEntryPtr);

    void
    do_close();

    void
    on_close(boost::system::error_code ec);

    void
    handleActivatePeer(const std::string& command, const std::string& message);

    void
    activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    
    void
    routeTransaction(keto::proto::MessageWrapper&  messageWrapper);

    void
    requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest);

    void
    pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage);

    void
    electBlockProducer();
    void
    electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper);
    void
    electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper);

    void
    pushRpcPeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    
    RpcPeer getPeer();
    bool isClosed();
    bool isActive();
    bool isRegistered();
    
private:
    bool reading;
    bool closed;
    bool active;
    bool registered;
    std::recursive_mutex classMutex;
    tcp::resolver resolver;
    websocket::stream<boostSsl::stream<tcp::socket>> ws_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::multi_buffer buffer_;
    ReadQueuePtr readQueuePtr;
    std::queue<std::shared_ptr<std::string>> queue_;

    //bool peered;
    RpcPeer rpcPeer;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    std::string accountHash;
    int sessionNumber;
    keto::election_common::ElectionResultCache electionResultCache;

    std::vector<uint8_t> buildHeloMessage();
    
    std::vector<uint8_t> buildConsensus(const keto::asn1::HashHelper& hashHelper);
    
    std::string buildMessage(const std::string& command, const std::string& message);
    std::string buildMessage(const std::string& command, const std::vector<uint8_t>& message);
    
    // response handling
    std::string helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash);
    void closeResponse(const std::string& command, const std::string& message);
    std::string consensusSessionResponse(const std::string& command, const std::string& sessionKey);
    std::string consensusResponse(const std::string& command, const std::string& message);
    std::string serverRequest(const std::string& command, const std::vector<uint8_t>& message);
    std::string serverRequest(const std::string& command, const std::string& message);
    void handlePeerResponse(const std::string& command, const std::string& message);

    // protocol methods
    std::string handleProtocolCheckRequest(const std::string& command, const std::string& message);
    void handleProtocolCheckAccept(const std::string& command, const std::string& message);
    void handleProtocolHeartbeat(const std::string& command, const std::string& message);

    // elect node request
    std::string handleElectionRequest(const std::string& command, const std::string& message);
    void handleElectionResponse(const std::string& command, const std::string& message);
    void handleElectionPublish(const std::string& command, const std::string& message);
    void handleElectionConfirmation(const std::string& command, const std::string& message);

    std::string handleRegisterResponse(const std::string& command, const std::string& message);
    std::string handleRequestNetworkSessionKeys(const std::string& command, const std::string& message);
    std::string handleRequestNetworkSessionKeysResponse(const std::string& command, const std::string& message);
    std::string handleRequestNetworkMasterKeyResponse(const std::string& command, const std::string& message);
    std::string handleRequestNetworkKeysResponse(const std::string& command, const std::string& message);
    std::string handleRequestNetworkFeesResponse(const std::string& command, const std::string& message);
    std::string handleRetryResponse(const std::string& command);
    std::string handleRegisterRequest(const std::string& command, const std::string& message);
    std::string handleTransaction(const std::string& command, const std::string& message);
    std::string handleBlock(const std::string& command, const std::string& message);
    std::string handleBlockSyncRequest(const std::string& command, const std::string& payload);
    std::string handleBlockSyncResponse(const std::string& command, const std::string& message);

    void fail(boost::system::error_code ec, const std::string& what);

    void send(const std::string& message);
    void sendMessage(std::shared_ptr<std::string> ss);
    void sendFirstQueueMessage();

    // change state
    void setClosed(bool closed);

    // set active
    void setActive(bool active);
};


}
}

#endif /* RPCSESSION_HPP */

