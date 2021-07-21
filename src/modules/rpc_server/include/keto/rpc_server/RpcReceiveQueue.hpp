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

#include "keto/rpc_server/RpcServerProtocol.hpp


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_server {

class RpcReceiveQueue;
typedef std::shared_ptr<RpcReceiveQueue> RpcReceiveQueuePtr;

class RpcReceiveQueue {
public:
    // the queue processor
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();


    RpcReceiveQueue();
    RpcReceiveQueue(const RpcReceiveQueue& orig) = delete;
    virtual ~RpcReceiveQueue();



private:
    RpcServerProtocolPtr rpcServerProtocolPtr;
    std::shared_ptr<std::thread> queueThreadPtr;

};
}
}


#endif //KETO_RPCRECEIVEQUEUE_HPP
