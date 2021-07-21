//
// Created by Brett Chaldecott on 2021/07/19.
//

#ifndef KETO_RPCSERVERMANAGER_HPP
#define KETO_RPCSERVERMANAGER_HPP

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

class RpcServerManager;
typedef std::shared_ptr<RpcServerManager> RpcServerManagerPtr;

class RpcServerManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcServerManager(const RpcServerManager& orig) = delete;
    virtual ~RpcServerManager();

    // account service management methods
    static RpcServerManagerPtr init();
    static void fin();
    static RpcServerManagerPtr getInstance();

    keto::event::Event routeTransaction(const keto::event::Event& event);
    keto::event::Event pushBlock(const keto::event::Event& event);
    keto::event::Event performNetworkSessionReset(const keto::event::Event& event);
    keto::event::Event performProtocoCheck(const keto::event::Event& event);
    keto::event::Event performConsensusHeartbeat(const keto::event::Event& event);
    keto::event::Event electBlockProducer(const keto::event::Event& event);
    keto::event::Event activatePeers(const keto::event::Event& event);
    keto::event::Event requestNetworkState(const keto::event::Event& event);
    keto::event::Event activateNetworkState(const keto::event::Event& event);
    keto::event::Event requestBlockSync(const keto::event::Event& event);

    keto::event::Event electBlockProducerPublish(const keto::event::Event& event);
    keto::event::Event electBlockProducerConfirmation(const keto::event::Event& event);


private:


    RpcServerManager();

};

}
}

#endif //KETO_RPCSERVERMANAGER_HPP
