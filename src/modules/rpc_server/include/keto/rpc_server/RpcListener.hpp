//
// Created by Brett Chaldecott on 2021/07/19.
//

#ifndef KETO_RPCLISTENER_HPP
#define KETO_RPCLISTENER_HPP

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

#include "keto/rpc_server/RpcSessionManager.hpp


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_server {

class RpcListener;

typedef std::shared_ptr<RpcListener> RpcListenerPtr;

class RpcListener : public std::enable_shared_from_this<RpcListener> {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcListener(
            std::shared_ptr<boost::asio::io_context> ioc,
            std::shared_ptr<sslBeast::context> ctx,
            tcp::endpoint endpoint);
    RpcListener(const RpcListener& orig) = delete;
    virtual ~RpcListener();




private:
    std::shared_ptr<boost::asio::io_context> ioc_;
    std::shared_ptr<sslBeast::context> ctx_;
    tcp::acceptor acceptor_;

};

}
}


#endif //KETO_RPCLISTENER_HPP
