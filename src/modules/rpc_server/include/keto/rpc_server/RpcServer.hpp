/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcServer.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 7:08 AM
 */

#ifndef RPCSERVER_HPP
#define RPCSERVER_HPP

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

class RpcServer;
typedef std::shared_ptr<RpcServer> RpcServerPtr;

typedef std::shared_ptr<boost::beast::multi_buffer> MultiBufferPtr;
typedef std::shared_ptr<std::lock_guard<std::mutex>> LockGuardPtr;
    
class RpcServer {
public:
    friend class session;
    
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    RpcServer();
    RpcServer(const RpcServer& orig) = delete;
    virtual ~RpcServer();
    
    // account service management methods
    static RpcServerPtr init();
    static void fin();
    static RpcServerPtr getInstance();
    
    
    void start();
    void stop();
    void setSecret(
            const keto::crypto::SecureVector& secret);
    
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

    bool isServerActive();

    std::string getExternalPeerInfo();
protected:
    keto::crypto::SecureVector getSecret();
    void setExternalIp(
            const boost::asio::ip::address& ipAddress);
    void setExternalHostname(
            const std::string& externalHostname);
    bool hasNetworkState();
    void enableNetworkState();
    
private:
    boost::asio::ip::address serverIp;
    boost::asio::ip::address externalIp;
    std::string externalHostname;
    unsigned short serverPort;
    int threads;
    std::shared_ptr<sslBeast::context> contextPtr;
    std::shared_ptr<net::io_context> ioc;
    std::vector<std::thread> threadsVector;
    keto::crypto::SecureVector secret;
    bool serverActive;
    bool networkState;
    
};

}
}

#endif /* RPCSERVER_HPP */

