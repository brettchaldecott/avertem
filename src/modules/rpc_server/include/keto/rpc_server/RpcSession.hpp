//
// Created by Brett Chaldecott on 2021/07/19.
//

#ifndef KETO_RPCSESSION_HPP
#define KETO_RPCSESSION_HPP

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

#include "keto/rpc_server/RpcSessionSocket.hpp"
#include "keto/rpc_server/RpcServerProtocol.hpp"
#include "keto/router_utils/RpcPeerHelper.hpp"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_server {

class RpcSession;
typedef std::shared_ptr<RpcSession> RpcSessionPtr;

class RpcSession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcSession(int sessionId, boost::asio::ip::tcp::socket&& socket, sslBeast::context& ctx);
    RpcSession(const RpcSession& orig) = delete;
    virtual ~RpcSession();

    void start();
    void stop();
    int getSessionId();
    std::string getAccount();
    std::string getAccountHash();
    void join();

    bool isActive();
    bool isRegistered();
    time_t getLastBlockTouch();

    void routeTransaction(keto::proto::MessageWrapper&  messageWrapper);
    void pushBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage);
    void performNetworkSessionReset();
    void performProtocolCheck();
    void performNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& protocolHeartbeatMessage);
    bool electBlockProducer();
    void activatePeer(const keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    void requestBlockSync(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest);
    void electBlockProducerPublish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper);
    void electBlockProducerConfirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper);


private:
    const int sessionId;
    RpcSessionSocketPtr rpcSessionSocketPtr;
    RpcServerProtocolPtr rpcServerProtocolPtr;


};

}
}


#endif //KETO_RPCSESSION_HPP
