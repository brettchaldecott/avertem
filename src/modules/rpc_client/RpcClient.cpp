//
// Created by Brett Chaldecott on 2021/08/10.
//

#include "keto/rpc_client/RpcClient.hpp"
#include "keto/rpc_client/RpcSessionManager.hpp"
#include "keto/rpc_client/RpcClientSession.hpp"
#include "keto/rpc_client/PeerStore.hpp"
#include "keto/rpc_client/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/TransactionHelper.hpp"
#include "keto/server_common/Constants.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

#include "keto/election_common/ElectionMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"


namespace keto {
namespace rpc_client {

static RpcClientPtr singleton;

std::string RpcClient::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcClient::RpcClient() : activated(false),networkState(false),activeNetworkState(false),peered(false)  {
    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::PEERS)) {
        this->configuredPeersString = config->getVariablesMap()[Constants::PEERS].
                as<std::string>();
    }

}

RpcClient::~RpcClient() {

}

RpcClientPtr RpcClient::init() {
    if (!singleton) {
        singleton = RpcClientPtr(new RpcClient());
    }
    return singleton;
}

void RpcClient::fin() {
    singleton.reset();
}

RpcClientPtr RpcClient::getInstance() {
    return singleton;
}

void RpcClient::start() {
    RpcSessionManager::getInstance()->start();
    PeerStore::init();
}

void RpcClient::postStart() {
    KETO_LOG_INFO << "The post start has been called";
    std::vector<std::string> peers = PeerStore::getInstance()->getPeers();
    if (!peers.size()) {
        KETO_LOG_INFO << "Use the configured peers : " << this->configuredPeersString;
        peers = keto::server_common::StringUtils(
                this->configuredPeersString).tokenize(",");
    } else {
        KETO_LOG_INFO << "Peers have been previously configured [" << peers.size() << "]";
        this->peered = true;
    }
    KETO_LOG_INFO << "After retrieving the peers.";
    for (std::vector<std::string>::iterator iter = peers.begin();
    iter != peers.end(); iter++) {
        KETO_LOG_INFO << "Connect to peer : " << (*iter);
        RpcPeer rpcPeer((*iter), this->peered);
        RpcSessionManager::getInstance()->addSession(rpcPeer);
    }
    RpcSessionManager::getInstance()->postStart();

    KETO_LOG_INFO << "[postStart] started the rpc client";
}

void RpcClient::preStop() {
}

void RpcClient::stop() {
    RpcSessionManager::getInstance()->stop();
    PeerStore::fin();
}

bool RpcClient::isActivated() {
    return this->activated;
}

// set the peers
void RpcClient::setPeers(const std::vector<std::string>& peers) {
    this->peered = true;
    PeerStore::getInstance()->setPeers(peers);

    KETO_LOG_INFO << "Connect ";
    for (std::string peer : peers) {
        KETO_LOG_INFO << "Connect to peer : " << peer;
        RpcPeer rpcPeer(peer, this->peered);
        RpcSessionManager::getInstance()->addSession(rpcPeer);
    }
}

// network status
bool RpcClient::hasNetworkState() {
    return this->networkState;
}

void RpcClient::setNetworkState(bool networkState) {
    this->networkState = networkState;
}

bool RpcClient::activateNetworkState() {
    return this->activeNetworkState;
}

keto::event::Event RpcClient::routeTransaction(const keto::event::Event& event) {

    keto::proto::MessageWrapper messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(messageWrapper);

    // check if there is a peer matching the target account hash this would be pure luck
    RpcClientSessionPtr rpcClientSessionPtr = RpcSessionManager::getInstance()->getSessionByAccount(messageWrapper.account_hash());
    if (rpcClientSessionPtr) {
        rpcClientSessionPtr->routeTransaction(messageWrapper);
    } else {
        // route to the default account which is the first peer in the list
        RpcClientSessionPtr rpcClientSessionPtr = RpcSessionManager::getInstance()->getFirstSession();
        if (rpcClientSessionPtr) {
            try {
                rpcClientSessionPtr->routeTransaction(messageWrapper);
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[RpcClient::routeTransaction] Failed route transaction : " << ex.what();
                KETO_LOG_ERROR << "[RpcClient::routeTransaction] Cause : " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[RpcClient::routeTransaction] Failed route transaction : " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[RpcClient::routeTransaction] Failed route transaction : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[RpcClient::routeTransaction] Failed route transaction : unknown cause";
            }
        } else {
            std::stringstream ss;
            ss << "No default route for [" <<
            messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
            BOOST_THROW_EXCEPTION(keto::rpc_client::NoDefaultRouteAvailableException(
                    ss.str()));
        }

    }

    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    std::stringstream ss;
    ss << "Routed to the server peer [" <<
    messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
    response.set_result(ss.str());
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event RpcClient::activatePeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    this->activated = rpcPeerHelper.isActive();
    for (RpcClientSessionPtr rpcClientSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions())
    {
        try {
            rpcClientSessionPtr->activatePeer(rpcPeerHelper);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::activatePeer] Failed to activate the peer : " << ex.what();
            KETO_LOG_ERROR << "[RpcClient::activatePeer] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::activatePeer] Failed to activate the peer : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::activatePeer] Failed to activate the peer : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcClient::activatePeer] Failed to activate the peer : unknown cause";
        }
    }
    return event;
}

keto::event::Event RpcClient::requestNetworkState(const keto::event::Event& event) {
    KETO_LOG_INFO << "[RpcClient::requestNetworkState] The client request network state has been set.";
    this->networkState = false;
    return event;
}

keto::event::Event RpcClient::activateNetworkState(const keto::event::Event& event) {
    KETO_LOG_INFO << "[RpcClient::activateNetworkState] The network status is active.";
    this->networkState = true;
    return event;
}

keto::event::Event RpcClient::requestBlockSync(const keto::event::Event& event) {
    keto::proto::SignedBlockBatchRequest request = keto::server_common::fromEvent<keto::proto::SignedBlockBatchRequest>(event);
    std::vector<RpcClientSessionPtr> rcpClientSessionPtrs = RpcSessionManager::getInstance()->getActiveSessions();
    KETO_LOG_INFO << "[RpcSessionManager::requestBlockSync] Making request to the following peers [" << rcpClientSessionPtrs.size() << "]";


    if (rcpClientSessionPtrs.size()) {
        // select a random rpc service if there are more than one upstream providers
        RpcClientSessionPtr currentSessionPtr = rcpClientSessionPtrs[0];
        if (rcpClientSessionPtrs.size()>1) {
            for (RpcClientSessionPtr rpcClientSessionPtr : rcpClientSessionPtrs) {
                if (rpcClientSessionPtr && (rpcClientSessionPtr->getLastBlockTouch() > currentSessionPtr->getLastBlockTouch())) {
                    currentSessionPtr = rpcClientSessionPtr;
                }
            }
        }
        try {
            currentSessionPtr->requestBlockSync(request);
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : "
            << ex.what();
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Cause : "
            << boost::diagnostic_information(ex, true);
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : "
            << boost::diagnostic_information(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : "
            << ex.what();
        } catch (...) {
            KETO_LOG_ERROR
            << "[RpcSessionManager::requestBlockSync] Failed to request a block sync : unknown cause";
        }
    } else {
        // attempt to call children for synchronization.
        KETO_LOG_INFO << "[RpcSessionManager::requestBlockSync] No upstream connections forcing the request down stream";
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                keto::server_common::Events::RPC_SERVER_REQUEST_BLOCK_SYNC,request));
    }

    return event;
}


keto::event::Event RpcClient::pushBlock(const keto::event::Event& event) {
    KETO_LOG_INFO << "[RpcClient::pushBlock] push block to client";
    for (RpcClientSessionPtr rpcClientSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {
        try {
            rpcClientSessionPtr->pushBlock(keto::server_common::fromEvent<keto::proto::SignedBlockWrapperMessage>(event));
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::pushBlock] Failed to push block : " << ex.what();
            KETO_LOG_ERROR << "[RpcClient::pushBlock] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::pushBlock] Failed to push block : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::pushBlock] Failed to push block : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcClient::pushBlock] Failed to push block : unknown cause";
        }
    }
    return event;
}


keto::event::Event RpcClient::electBlockProducer(const keto::event::Event& event) {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());

    keto::election_common::ElectionMessageProtoHelper electionMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionMessage>(event));

    std::vector<RpcClientSessionPtr> sessions = RpcSessionManager::getInstance()->getActiveSessions();
    for (int index = 0; (index < keto::server_common::Constants::ELECTION::ELECTOR_COUNT) && (sessions.size()); index++) {

        // distribution
        RpcClientSessionPtr rpcClientSessionPtr;
        if (sessions.size() > 1) {
            std::uniform_int_distribution<int> distribution(0, sessions.size() - 1);
            distribution(stdGenerator);
            int pos = distribution(stdGenerator);
            rpcClientSessionPtr = sessions[pos];
            sessions.erase(sessions.begin() + pos);
        } else {
            rpcClientSessionPtr = sessions[0];
            sessions.clear();
        }

        // get the account
        try {
            rpcClientSessionPtr->electBlockProducer();
            electionMessageProtoHelper.addAccount(keto::asn1::HashHelper(rpcClientSessionPtr->getAccountHash()));
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducer] Failed to push block : " << ex.what();
            KETO_LOG_ERROR << "[RpcClient::electBlockProducer] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducer] Failed to push block : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducer] Failed to push block : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducer] Failed to push block : unknown cause";
        }
    }

    return keto::server_common::toEvent<keto::proto::ElectionMessage>(electionMessageProtoHelper);
}

keto::event::Event RpcClient::electBlockProducerPublish(const keto::event::Event& event) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));

    for (RpcClientSessionPtr rpcClientSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {

        // get the account
        try {
            rpcClientSessionPtr->electBlockProducerPublish(electionPublishTangleAccountProtoHelper);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerPublish] Failed to publish the tangle change: " << ex.what();
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerPublish] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerPublish] Failed to publish the tangle changes : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerPublish] Failed to publish the tangle changes : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerPublish] Failed to publish the tangle changes : unknown cause";
        }
    }

    return event;
}

keto::event::Event RpcClient::electBlockProducerConfirmation(const keto::event::Event& event) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));

    for (RpcClientSessionPtr rpcClientSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {

        // get the account
        try {
            rpcClientSessionPtr->electBlockProducerConfirmation(electionConfirmationHelper);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerConfirmation] Failed to publish the tangle change: " << ex.what();
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerConfirmation] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerConfirmation] Failed to publish the tangle changes : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerConfirmation] Failed to publish the tangle changes : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcClient::electBlockProducerConfirmation] Failed to publish the tangle changes : unknown cause";
        }
    }

    return event;
}

keto::event::Event RpcClient::pushRpcPeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    for (RpcClientSessionPtr rpcClientSessionPtr : RpcSessionManager::getInstance()->getRegisteredSessions()) {

        try {
            rpcClientSessionPtr->pushRpcPeer(rpcPeerHelper);
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::pushToRpcPeer] Failed to push peer to rpc peers: " << ex.what();
            KETO_LOG_ERROR << "[RpcClient::pushToRpcPeer] Cause : " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::pushToRpcPeer] Failed to push peer to rpc peers : " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[RpcClient::pushToRpcPeer] Failed to push peer to rpc peers : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[RpcClient::pushToRpcPeer] Failed to push peer to rpc peers : unknown cause";
        }
    }

    return event;
}

}
}