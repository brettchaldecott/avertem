//
// Created by Brett Chaldecott on 2021/07/20.
//

#ifndef KETO_RPCRECEIVEQUEUE_HPP
#define KETO_RPCRECEIVEQUEUE_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "keto/common/MetaInfo.hpp"
#include "keto/crypto/Containers.hpp"

#include "keto/event/Event.hpp"

#include "keto/rpc_server/RpcSendQueue.hpp"
#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"
#include "keto/election_common/ElectionResultCache.hpp"
#include "keto/election_common/ElectionUtils.hpp"


#include "SoftwareConsensus.pb.h"
#include "HandShake.pb.h"
#include "Route.pb.h"
#include "BlockChain.pb.h"
#include "Protocol.pb.h"
#include "KeyStore.pb.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_server {

class RpcReadQueueEntry {
public:
    RpcReadQueueEntry(const std::string& command, const std::string& payload) :
            command(command),payload(payload){}

    RpcReadQueueEntry(const RpcReadQueueEntry& orig) = delete;
    virtual ~RpcReadQueueEntry() {}

    std::string getCommand() {return this->command;}
    std::string getPayload() {return this->payload;}

private:
    std::string command;
    std::string payload;
};
typedef std::shared_ptr<RpcReadQueueEntry> RpcReadQueueEntryPtr;

class RpcReceiveQueue;
typedef std::shared_ptr<RpcReceiveQueue> RpcReceiveQueuePtr;

class RpcReceiveQueue {
public:
    // the queue processor
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class RpcSession;

    RpcReceiveQueue(int sessionId);
    RpcReceiveQueue(const RpcReceiveQueue& orig) = delete;
    virtual ~RpcReceiveQueue();

    void start(const RpcSendQueuePtr& rpcSendQueuePtr);
    void preStop();
    void stop();
    void abort();
    void join();
    bool clientIsActive();
    bool isActive();
    bool isRegistered();

    void pushEntry(const std::string& command, const std::string& payload);
    void performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage);

    // block touch methods
    long getLastBlockTouch();
    long blockTouch();

    std::string getAccountHash();
    std::string getAccountHashStr();

private:
    // private member variables
    int sessionId;
    bool active;
    bool aborted;
    bool clientActive;
    bool registered;
    keto::election_common::ElectionResultCache electionResultCache;
    time_t lastBlockTouch;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    RpcSendQueuePtr rpcSendQueuePtr;
    keto::crypto::SecureVector sessionIdentifier;
    std::shared_ptr<Botan::AutoSeeded_RNG> generatorPtr;
    std::shared_ptr<std::thread> queueThreadPtr;
    std::deque<RpcReadQueueEntryPtr> readQueue;
    std::shared_ptr<keto::rpc_protocol::ServerHelloProtoHelper> serverHelloProtoHelperPtr;


    // address variables
    boost::asio::ip::address localAddress;
    std::string localHostname;
    std::string remoteAddress;
    std::string remoteHostname;

    // client status methods
    void setClientActive(bool clientActive);
    std::string getAccount();

    // processing methods
    void run();
    RpcReadQueueEntryPtr popEntry();
    void processEntry(const RpcReadQueueEntryPtr& entry);

    // protocol methods
    void handleBlockSyncRequest(const std::string& command, const std::string& payload);
    void handleHello(const std::string& command, const std::string& payload);
    void handleRetryResponse(const std::string& command);
    void handleHelloConsensus(const std::string& command, const std::string& payload);
    void handlePeer(const std::string& command, const std::string& payload);
    void handleRegister(const std::string& command, const std::string& payload);
    void handlePushRpcPeers(const std::string& command, const std::string& payload);
    void handleActivate(const std::string& command, const std::string& payload);
    void handleTransaction(const std::string& command, const std::string& payload);
    void handleTransactionProcessed(const std::string& command, const std::string& payload);
    void handleRequestNetworkSessionKeys(const std::string& command, const std::string& payload);
    void handleRequestMasterNetworkKeys(const std::string& command, const std::string& payload);
    void handleRequestNetworkKeys(const std::string& command, const std::string& payload);
    void handleRequestNetworkFees(const std::string& command, const std::string& payload);
    void handleClientNetworkComplete(const std::string& command, const std::string& payload);
    void handleBlockPush(const std::string& command, const std::string& payload);
    void handleBlockSyncResponse(const std::string& command, const std::string& payload);
    void handleMissingBlockSyncRequest(const std::string& command, const std::string& payload);
    void handleMissingBlockSyncResponse(const std::string& command, const std::string& payload);
    void handleProtocolCheckResponse(const std::string& command, const std::string& payload);
    void handleElectionRequest(const std::string& command, const std::string& payload);
    void handleElectionResponse(const std::string& command, const std::string& payload);
    void handleElectionPublish(const std::string& command, const std::string& message);
    void handleElectionConfirmation(const std::string& command, const std::string& message);
    void handleResponseNetworkStatus(const std::string& command, const std::string& payload);

    // session secret
    keto::crypto::SecureVector generateSession();

};
}
}


#endif //KETO_RPCRECEIVEQUEUE_HPP
