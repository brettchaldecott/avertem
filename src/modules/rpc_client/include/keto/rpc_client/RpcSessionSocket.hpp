//
// Created by Brett Chaldecott on 2021/08/11.
//

#ifndef KETO_RPCSESSIONSOCKET_H
#define KETO_RPCSESSIONSOCKET_H

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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_client {

class RpcClientProtocol;
typedef std::shared_ptr<RpcClientProtocol> RpcClientProtocolPtr;

class RpcSessionSocket;
typedef std::shared_ptr<RpcSessionSocket> RpcSessionSocketPtr;

class RpcSessionSocket : public std::enable_shared_from_this<RpcSessionSocket> {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcSessionSocket(int sessionId,
                     std::shared_ptr<net::io_context>& ioc,
                     std::shared_ptr<sslBeast::context>& ctx,
                     const RpcPeer& rpcPeer);
    RpcSessionSocket(const RpcSessionSocket& orig) = delete;
    virtual ~RpcSessionSocket();

    void start(const RpcClientProtocolPtr& rpcClientProtocolPtr);
    bool isActive();
    void stop();
    void join();
    void send(const std::string& message);

    bool isReconnect();
    void setReconnect(bool reconnect);

    bool isClosed();
    void setClosed();

private:
    const int sessionId;
    bool active;
    bool reconnect;
    bool closed;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    tcp::resolver resolver;
    websocket::stream<
        beast::ssl_stream<beast::tcp_stream>> ws_;

    boost::beast::flat_buffer sessionBuffer;
    std::string outMessage;
    RpcClientProtocolPtr rpcClientProtocolPtr;

    RpcPeer rpcPeer;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    //int sessionNumber;
    keto::election_common::ElectionResultCache electionResultCache;


    void deactivate();

    void run();
    void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results);
    void on_connect(boost::system::error_code ec, tcp::resolver::results_type::endpoint_type);

    void on_ssl_handshake(boost::system::error_code ec);

    void on_handshake(boost::system::error_code ec);


    void sendMessage();
    void on_write(boost::system::error_code ec, std::size_t bytes_transferred);

    void do_read();

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

    void on_close(boost::system::error_code ec);

    void fail(boost::system::error_code ec,const std::string& what);

};

}
}


#endif //KETO_RPCSESSIONSOCKET_H
