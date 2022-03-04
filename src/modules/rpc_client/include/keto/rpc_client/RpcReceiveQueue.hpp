//
// Created by Brett Chaldecott on 2021/08/14.
//

#ifndef KETO_RPCRECEIVEQUEUE_HPP
#define KETO_RPCRECEIVEQUEUE_HPP

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
#include <boost/beast/ssl.hpp>

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
#include "keto/election_common/PublishedElectionInformationHelper.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"
#include "keto/election_common/ElectionResultCache.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/common/MetaInfo.hpp"

#include "keto/rpc_client/RpcPeer.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_client {

class RpcReadQueueEntry {
public:
    RpcReadQueueEntry(const std::string& command, const std::string& payload, const std::string& misc) :
    command(command),payload(payload), misc(misc){}

    RpcReadQueueEntry(const RpcReadQueueEntry& orig) = delete;
    virtual ~RpcReadQueueEntry() {}

    std::string getCommand() {return this->command;}
    std::string getPayload() {return this->payload;}
    std::string getMisc() {return this->misc;}

private:
    std::string command;
    std::string payload;
    std::string misc;
};
typedef std::shared_ptr<RpcReadQueueEntry> RpcReadQueueEntryPtr;

class RpcReceiveQueue;
typedef std::shared_ptr<RpcReceiveQueue> RpcReceiveQueuePtr;

class RpcSendQueue;
typedef std::shared_ptr<RpcSendQueue> RpcSendQueuePtr;

class RpcReceiveQueue {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcReceiveQueue(int sessionId, const RpcPeer& rpcPeer);
    RpcReceiveQueue(const RpcReceiveQueue& origin) = delete;
    virtual ~RpcReceiveQueue();

    void start(const RpcSendQueuePtr& rpcSendQueuePtr);
    void preStop();
    void stop();
    void abort();
    void join();
    bool clientIsActive();
    void setClientActive(bool clientActive);
    bool isActive();
    bool isRegistered();
    bool containsPublishAccount(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper);
    bool containsConfirmationAccount(const keto::asn1::HashHelper& hashHelper);

    void pushEntry(const std::string& command, const std::string& payload, const std::string& misc);

    // block touch methods
    long getLastBlockTouch();
    long blockTouch();

    int getSessionId();
    std::string getHost();
    std::string getAccountHash();
    std::string getAccountHashStr();


private:
    // private member variables
    int sessionId;
    bool active;
    bool clientActive;
    bool aborted;
    bool registered;
    time_t lastBlockTouch;

    RpcPeer rpcPeer;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    std::string accountHash;
    std::string externalHostname;

    keto::election_common::ElectionResultCache electionResultCache;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    RpcSendQueuePtr rpcSendQueuePtr;
    keto::crypto::SecureVector sessionIdentifier;
    std::shared_ptr<Botan::AutoSeeded_RNG> generatorPtr;
    std::shared_ptr<std::thread> queueThreadPtr;
    std::deque<RpcReadQueueEntryPtr> readQueue;


    // processing methods
    void run();
    RpcReadQueueEntryPtr popEntry();
    void processEntry(const RpcReadQueueEntryPtr& entry);


    void handleBlockSyncRequest(const std::string& command, const std::string& payload);
    void handleHelloRequest(const std::string& command, const std::string& payload);
    void helloConsensusResponse(const std::string& command, const std::string& sessionKey, const std::string& initHash);
    void handleRetryResponse(const std::string& command);
    void helloAcceptedResponse(const std::string& command, const std::string& payload);
    void handlePeerRequest(const std::string& command, const std::string& payload);
    void handleRequestNetworkSessionKeys(const std::string& command, const std::string& payload);
    void handleRegisterRequest(const std::string& command, const std::string& payload);
    void handlePeerResponse(const std::string& command, const std::string& payload);
    void handleRegisterResponse(const std::string& command, const std::string& payload);
    void handleActivatePeer(const std::string& command, const std::string& payload);
    void handleTransaction(const std::string& command, const std::string& payload);
    void handleBlock(const std::string& command, const std::string& payload);
    void consensusSessionResponse(const std::string& command, const std::string& sessionKey);
    void consensusResponse(const std::string& command, const std::string& payload);
    void handleRequestNetworkSessionKeysResponse(const std::string& command, const std::string& payload);
    void handleRequestNetworkMasterKeyResponse(const std::string& command, const std::string& payload);
    void handleRequestNetworkKeysResponse(const std::string& command, const std::string& payload);
    void handleRequestNetworkFeesResponse(const std::string& command, const std::string& payload);
    void closeResponse(const std::string& command, const std::string& payload);
    void handleBlockSyncResponse(const std::string& command, const std::string& payload);
    void handleMissingBlockSyncRequest(const std::string& command, const std::string& payload);
    void handleMissingBlockSyncResponse(const std::string& command, const std::string& payload);
    void handleProtocolCheckRequest(const std::string& command, const std::string& payload);
    void handleProtocolCheckAccept(const std::string& command, const std::string& payload);
    void handleProtocolHeartbeat(const std::string& command, const std::string& payload);
    void handleElectionRequest(const std::string& command, const std::string& payload);
    void handleElectionResponse(const std::string& command, const std::string& payload);
    void handleElectionPublish(const std::string& command, const std::string& message);
    void handleElectionConfirmation(const std::string& command, const std::string& payload);
    void handleRequestNetworkStatus(const std::string& command, const std::string& payload);
    void handleResponseNetworkStatus(const std::string& command, const std::string& payload);
    void handleInternalException(const std::string& command, const std::string& cause = "");

    // consensus messages
    std::vector<uint8_t> buildConsensus(const keto::asn1::HashHelper& hashHelper);
    std::vector<uint8_t> buildHeloMessage();
};

}
}


#endif //KETO_RPCRECEIVEQUEUE_HPP
