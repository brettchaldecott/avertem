//
// Created by Brett Chaldecott on 2021/08/11.
//

#ifndef KETO_RPCCLIENTSESSION_H
#define KETO_RPCCLIENTSESSION_H

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
#include "keto/rpc_client/RpcClientProtocol.hpp"
#include "keto/rpc_client/RpcSessionSocket.hpp"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_client {

class RpcClientSession;
typedef std::shared_ptr<RpcClientSession> RpcClientSessionPtr;

class RpcClientSession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcClientSession(int sessionId,
                     std::shared_ptr<net::io_context>& ioc,
                     std::shared_ptr<sslBeast::context>& ctx,
                     const RpcPeer& rpcPeer);
    RpcClientSession(const RpcClientSession& orig) = delete;
    virtual ~RpcClientSession();

    void start();
    void stop();
    int getSessionId();
    std::string getAccount();
    std::string getAccountHash();
    std::string getHost();
    void join();


    bool isRegistered();
    bool isActive();
    bool isReconnect();
    RpcPeer getRpcPeer();

    time_t getLastBlockTouch();

    // internal methods
    void routeTransaction(keto::proto::MessageWrapper&  messageWrapper);
    void activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    void requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest);
    void pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage);
    void electBlockProducer();
    void electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper);
    void electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper);
    void pushRpcPeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper);

private:
    const int sessionId;
    RpcPeer rpcPeer;
    RpcClientProtocolPtr rpcClientProtocolPtr;
    RpcSessionSocketPtr rpcSessionSocketPtr;


};

}
}

#endif //KETO_RPCCLIENTSESSION_H
