//
// Created by Brett Chaldecott on 2021/07/24.
//

#ifndef KETO_RPCSESSIONSOCKET_HPP
#define KETO_RPCSESSIONSOCKET_HPP

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


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

class RpcSessionSocket;
typedef std::shared_ptr<RpcSessionSocket> RpcSessionSocketPtr;

class RpcServerProtocol;
typedef std::shared_ptr<RpcServerProtocol> RpcServerProtocolPtr;

class RpcSessionSocket : public std::enable_shared_from_this<RpcSessionSocket> {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    friend class RpcSendQueue;

    RpcSessionSocket(int sessionId,tcp::socket&& socket, sslBeast::context& ctx);
    RpcSessionSocket(const RpcSessionSocket& orig) = delete;
    virtual ~RpcSessionSocket();

    void start(const RpcServerProtocolPtr& rpcServerProtocolPtr);
    bool isActive();
    void stop();
    void join();
    void send(const std::string& message);

    void setClosed();
    bool isClosed();

private:
    int sessionId;
    bool active;
    bool closed;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    websocket::stream<
        beast::ssl_stream<beast::tcp_stream>> ws_;
    boost::beast::flat_buffer sessionBuffer;
    std::string outMessage;
    RpcServerProtocolPtr rpcServerProtocolPtr;

    void deactivate();

    void run();
    void on_handshake(boost::system::error_code ec);
    void on_accept(boost::system::error_code ec);
    void do_read();
    void on_read(
            boost::system::error_code ec,
            std::size_t bytes_transferred);
    void sendMessage();
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(boost::system::error_code ec);
    void fail(boost::system::error_code ec, char const* what);

};

}
}

#endif //KETO_RPCSESSIONSOCKET_HPP
