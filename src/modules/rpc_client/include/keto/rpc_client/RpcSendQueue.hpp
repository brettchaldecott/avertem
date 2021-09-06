//
// Created by Brett Chaldecott on 2021/08/14.
//

#ifndef KETO_RPCSENDQUEUE_HPP
#define KETO_RPCSENDQUEUE_HPP

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

class RpcSendQueueEntry {
public:
    RpcSendQueueEntry(const std::string& command, const std::string& payload) :
    command(command),payload(payload){}

    RpcSendQueueEntry(const RpcSendQueueEntry& orig) = delete;
    virtual ~RpcSendQueueEntry() {}

    std::string getCommand() {return this->command;}
    std::string getPayload() {return this->payload;}

private:
    std::string command;
    std::string payload;
};
typedef std::shared_ptr<RpcSendQueueEntry> RpcSendQueueEntryPtr;

class RpcSendQueue;
typedef std::shared_ptr<RpcSendQueue> RpcSendQueuePtr;

class RpcSessionSocket;
typedef std::shared_ptr<RpcSessionSocket> RpcSessionSocketPtr;

class RpcSendQueue {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcSendQueue(int sessionId);
    RpcSendQueue(const RpcSendQueue& origin) = delete;
    virtual ~RpcSendQueue();

    void start(const RpcSessionSocketPtr& rpcSessionSocketPtr);
    void preStop();
    void stop();
    void abort();
    void join();

    void pushEntry(const std::string& command, const std::string& payload);
    void releaseEntry();

private:
    int sessionId;
    bool active;
    bool closed;
    bool aborted;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    RpcSessionSocketPtr rpcSessionSocketPtr;
    std::shared_ptr<std::thread> queueThreadPtr;
    std::deque<RpcSendQueueEntryPtr> sendQueue;
    RpcSendQueueEntryPtr activeEntry;

    void run();
    RpcSendQueueEntryPtr peekEntry();
    void _pushEntry(const std::string& command, const std::string& payload);
    void processEntry(const RpcSendQueueEntryPtr& entry);

};

}
}


#endif //KETO_RPCSENDQUEUE_HPP
