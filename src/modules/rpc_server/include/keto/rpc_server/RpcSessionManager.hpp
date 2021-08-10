//
// Created by Brett Chaldecott on 2021/07/19.
//

#ifndef KETO_RPCSESSIONMANAGER_HPP
#define KETO_RPCSESSIONMANAGER_HPP

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

#include "keto/rpc_server/RpcSession.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_server {

class RpcSessionManager;
typedef std::shared_ptr<RpcSessionManager> RpcSessionManagerPtr;

class RpcSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    friend class RpcSession;

    RpcSessionManager();
    RpcSessionManager(const RpcSessionManager& orig) = delete;
    virtual ~RpcSessionManager();

    // account service management methods
    static RpcSessionManagerPtr init();
    static void fin();
    static RpcSessionManagerPtr getInstance();

    void start();
    void stop();

    RpcSessionPtr addSession(boost::asio::ip::tcp::socket&& socket, sslBeast::context& _ctx);
    RpcSessionPtr getSession(int sessionId);
    RpcSessionPtr getSession(const std::string& account);
    void markAsEndedSession(int sessionId);
    std::vector<RpcSessionPtr> getSessions();
    std::vector<RpcSessionPtr> getRegisteredSessions();
    std::vector<RpcSessionPtr> getActiveSessions();


private:
    bool active;
    bool serverActive;
    bool networkState;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    int sessionSequence;
    std::map<int,RpcSessionPtr> sessionMap;
    std::deque<RpcSessionPtr> garbageDeque;
    std::shared_ptr<std::thread> sessionManagerThreadPtr;

    void run();
    RpcSessionPtr popGarbageSession();
    void deactivate();



};

}
}

#endif //KETO_RPCSESSIONMANAGER_HPP
