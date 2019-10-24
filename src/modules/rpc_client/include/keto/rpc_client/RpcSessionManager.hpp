/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcSessionManager.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 1:18 PM
 */

#ifndef RPCSESSIONMANAGER_HPP
#define RPCSESSIONMANAGER_HPP

#include "keto/rpc_client/RpcSession.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <thread>
#include <vector>

#include "keto/common/MetaInfo.hpp"

#include "keto/event/Event.hpp"

#include "keto/rpc_client/RpcPeer.hpp"
#include "keto/rocks_db/DBManager.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_client {

class RpcSessionManager;
typedef std::shared_ptr<RpcSessionManager> RpcSessionManagerPtr;

class RpcSessionManager {
public:
    friend class RpcSession;
    
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    RpcSessionManager();
    RpcSessionManager(const RpcSessionManager& orig) = delete;
    virtual ~RpcSessionManager();
    
    // account service management methods
    static RpcSessionManagerPtr init();
    static void fin();
    static RpcSessionManagerPtr getInstance();
    
    // the list of peers
    std::vector<std::string> listPeers();
    std::vector<std::string> listAccountPeers();
    
    void start();
    void postStart();
    void stop();

    keto::event::Event activatePeer(const keto::event::Event& event);
    keto::event::Event requestBlockSync(const keto::event::Event& event);
    keto::event::Event routeTransaction(const keto::event::Event& event);
    keto::event::Event pushBlock(const keto::event::Event& event);


    keto::event::Event electBlockProducer(const keto::event::Event& event);
    keto::event::Event electBlockProducerPublish(const keto::event::Event& event);
    keto::event::Event electBlockProducerConfirmation(const keto::event::Event& event);

    keto::event::Event pushRpcPeer(const keto::event::Event& event);
    bool isActivated();
    
    
protected:
    void setPeers(const std::vector<std::string>& peers, bool peered = true);
    void reconnect(const RpcPeer& rpcPeer);
    void setAccountSessionMapping(const std::string& account,
            const RpcSessionPtr& rpcSessionPtr);
    void removeAccountSessionMapping(const std::string& account);
private:
    std::recursive_mutex classMutex;
    std::map<std::string,RpcSessionPtr> sessionMap;
    std::map<std::string,RpcSessionPtr> accountSessionMap;
    // The io_context is required for all I/O
    std::shared_ptr<net::io_context> ioc;
    // The SSL context is required, and holds certificates
    std::shared_ptr<sslBeast::context> ctx;
    // the thread information
    std::string configuredPeersString;
    int threads;
    std::vector<std::thread> threadsVector;
    bool peered;
    bool activated;


    bool hasAccountSessionMapping(const std::string& account);
    RpcSessionPtr getAccountSessionMapping(const std::string& account);
    RpcSessionPtr getDefaultPeer();
    RpcSessionPtr getActivePeer();
    bool registeredAccounts();
};


}
}

#endif /* RPCSESSIONMANAGER_HPP */

