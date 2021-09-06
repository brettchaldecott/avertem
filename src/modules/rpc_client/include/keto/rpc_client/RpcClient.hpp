//
// Created by Brett Chaldecott on 2021/08/10.
//

#ifndef KETO_RPCCLIENT_H
#define KETO_RPCCLIENT_H

/*
 * File:   RpcSession.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 12:32 PM
 */

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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace keto {
namespace rpc_client {

class RpcClient;
typedef std::shared_ptr<RpcClient> RpcClientPtr;

class RpcClient {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcClient();
    RpcClient(const RpcClient& rpcClient) = delete;
    virtual ~RpcClient();

    static RpcClientPtr init();
    static void fin();
    static RpcClientPtr getInstance();

    void start();
    void postStart();
    void preStop();
    void stop();

    bool isActivated();

    // set the peers
    void setPeers(const std::vector<std::string>& peers);

    // network status
    bool hasNetworkState();
    void setNetworkState(bool networkState);
    bool activateNetworkState();


    // event methods
    keto::event::Event routeTransaction(const keto::event::Event& event);
    keto::event::Event activatePeer(const keto::event::Event& event);
    keto::event::Event requestNetworkState(const keto::event::Event& event);
    keto::event::Event activateNetworkState(const keto::event::Event& event);
    keto::event::Event requestBlockSync(const keto::event::Event& event);
    keto::event::Event pushBlock(const keto::event::Event& event);
    keto::event::Event electBlockProducer(const keto::event::Event& event);
    keto::event::Event electBlockProducerPublish(const keto::event::Event& event);
    keto::event::Event electBlockProducerConfirmation(const keto::event::Event& event);
    keto::event::Event pushRpcPeer(const keto::event::Event& event);

private:
    std::mutex classMutex;
    std::condition_variable stateCondition;
    bool activated;
    bool networkState;
    bool activeNetworkState;
    bool peered;
    std::string configuredPeersString;


};

}
}


#endif //KETO_RPCCLIENT_H
