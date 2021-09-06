//
// Created by Brett Chaldecott on 2021/08/13.
//

#ifndef KETO_RPCCLIENTPROTOCOL_H
#define KETO_RPCCLIENTPROTOCOL_H

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

class RpcSessionSocket;
typedef std::shared_ptr<RpcSessionSocket> RpcSessionSocketPtr;

class RpcReceiveQueue;
typedef std::shared_ptr<RpcReceiveQueue> RpcReceiveQueuePtr;

class RpcSendQueue;
typedef std::shared_ptr<RpcSendQueue> RpcSendQueuePtr;

class RpcClientProtocol;
typedef std::shared_ptr<RpcClientProtocol> RpcClientProtocolPtr;

class RpcClientProtocol {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcClientProtocol(int sessionId,const RpcPeer& rpcPeer);
    RpcClientProtocol(const RpcClientProtocol& orig) = delete;
    virtual ~RpcClientProtocol();

    void start(const RpcSessionSocketPtr& rpcSessionSocket);
    void preStop();
    void stop();
    void abort();
    void join();
    bool isStarted();

    RpcReceiveQueuePtr getReceiveQueue();
    RpcSendQueuePtr getSendQueue();

private:
    std::mutex classMutex;
    int sessionId;
    bool started;
    RpcReceiveQueuePtr rpcReceiveQueuePtr;
    RpcSendQueuePtr rpcSendQueuePtr;

};

}
}

#endif //KETO_RPCCLIENTPROTOCOL_H
