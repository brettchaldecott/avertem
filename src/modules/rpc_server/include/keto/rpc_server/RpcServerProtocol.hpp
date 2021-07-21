//
// Created by Brett Chaldecott on 2021/07/19.
//

#ifndef KETO_RPCSERVERPROTOCOL_HPP
#define KETO_RPCSERVERPROTOCOL_HPP

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

class RpcServerProtocol;
typedef std::shared_ptr<RpcServerProtocol> RpcServerProtocolPtr;

class RpcServerProtocol : public std::enable_shared_from_this<RpcServerProtocol> {
public:
    // hearder version
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class RpcServerProtocol;

    // the rpc sever protocl starter
    RpcServerProtocol();
    RpcServerProtocol(const RpcServerProtocol& orig) = delete;
    virtual ~RpcServerProtocol();



private:


};

}
}

#endif //KETO_RPCSERVERPROTOCOL_HPP
