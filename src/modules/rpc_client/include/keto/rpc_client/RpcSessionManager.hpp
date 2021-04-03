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
    std::vector<keto::rpc_client::RpcSessionWrapperPtr> getPeers();
    std::vector<std::string> listPeers();
    std::vector<std::string> listAccountPeers();
    
    void start();
    void postStart();
    void preStop();
    void stop();

    keto::event::Event activatePeer(const keto::event::Event& event);
    keto::event::Event requestNetworkState(const keto::event::Event& event);
    keto::event::Event activateNetworkState(const keto::event::Event& event);
    keto::event::Event requestBlockSync(const keto::event::Event& event);
    keto::event::Event routeTransaction(const keto::event::Event& event);
    keto::event::Event pushBlock(const keto::event::Event& event);


    keto::event::Event electBlockProducer(const keto::event::Event& event);
    keto::event::Event electBlockProducerPublish(const keto::event::Event& event);
    keto::event::Event electBlockProducerConfirmation(const keto::event::Event& event);

    keto::event::Event pushRpcPeer(const keto::event::Event& event);
    bool isActivated();

    bool hasNetworkState();
    void activateNetworkState();
    bool isTerminated();
    void terminate();

    //int incrementSessionCount();
    //int decrementSessionCount();

protected:
    void setPeers(const std::vector<std::string>& peers, bool peered = true);
    void reconnect(RpcPeer& rpcPeer);
    void setAccountSessionMapping(const std::string& peer, const std::string& account);
    void removeSession(const RpcPeer& rpcPeer, const std::string& account);
private:
    int sessionSequence;
    std::recursive_mutex classMutex;
    std::mutex sessionClassMutex;
    std::condition_variable stateCondition;
    std::map<std::string,RpcSessionWrapperPtr> sessionMap;
    std::map<std::string,RpcSessionWrapperPtr> accountSessionMap;
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
    bool terminated;
    bool networkState;
    int sessionCount;


    bool hasAccountSessionMapping(const std::string& account);
    RpcSessionWrapperPtr getAccountSessionMapping(const std::string& account);
    RpcSessionWrapperPtr getDefaultPeer();
    RpcSessionWrapperPtr getActivePeer();
    std::vector<RpcSessionWrapperPtr> getActivePeers();
    std::vector<RpcSessionWrapperPtr> getAccountPeers();
    bool registeredAccounts();

    void waitForSessionEnd();
    int getSessionCount(bool waitForTimeout);
    void clearSessions();
    int getNextSessionId();
};


}
}

#endif /* RPCSESSIONMANAGER_HPP */

