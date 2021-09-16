//
// Created by Brett Chaldecott on 2021/08/11.
//

#ifndef KETO_RPCSESSIONMANAGER_HPP
#define KETO_RPCSESSIONMANAGER_HPP

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

#include "keto/rpc_client/RpcPeer.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_client {

class RpcClientSession;
typedef std::shared_ptr<RpcClientSession> RpcClientSessionPtr;

class RpcSessionManager;
typedef std::shared_ptr<RpcSessionManager> RpcSessionManagerPtr;

class RpcSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();


    friend class RpcClientSession;

    RpcSessionManager();
    RpcSessionManager(const RpcSessionManager& orig) = delete;
    virtual ~RpcSessionManager();

    // account service management methods
    static RpcSessionManagerPtr init();
    static void fin();
    static RpcSessionManagerPtr getInstance();


    void start();
    void postStart();
    void preStop();
    void stop();


    RpcClientSessionPtr addSession(const RpcPeer& rpcPeer);
    RpcClientSessionPtr getFirstSession();
    RpcClientSessionPtr getSession(int sessionId);
    RpcClientSessionPtr getSessionByAccount(const std::string& account);
    RpcClientSessionPtr getSessionByHost(const std::string& host);
    void markAsEndedSession(int sessionId);
    std::vector<RpcClientSessionPtr> getSessions();
    std::vector<RpcClientSessionPtr> getRegisteredSessions();
    std::vector<RpcClientSessionPtr> getActiveSessions();
    bool isActive();

private:
    bool active;
    std::mutex classMutex;
    std::condition_variable stateCondition;
    int sessionSequence;
    int threads;
    std::vector<std::thread> threadsVector;
    std::map<int,RpcClientSessionPtr> sessionMap;
    std::deque<RpcClientSessionPtr> garbageDeque;
    std::shared_ptr<std::thread> sessionManagerThreadPtr;

    // The io_context is required for all I/O
    std::shared_ptr<net::io_context> ioc;
    // The SSL context is required, and holds certificates
    std::shared_ptr<sslBeast::context> ctx;

    void run();
    RpcClientSessionPtr popGarbageSession();
    void deactivate();

};

}
}

#endif //KETO_RPCSESSIONMANAGER_HPP
