/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.cpp
 * Author: ubuntu
 * 
 * Created on March 3, 2018, 10:25 AM
 */

#include "keto/router/EventRegistry.hpp"
#include "keto/router/RouterService.hpp"
#include "keto/router/ConsensusService.hpp"


#include "keto/software_consensus/ConsensusStateManager.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/router/ConsensusService.hpp"
#include "keto/router/PeerCache.hpp"
#include "keto/router/TangleServiceCache.hpp"

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"

#include "keto/chain_query_common/ProducerResultProtoHelper.hpp"

namespace keto {
namespace router {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}

keto::event::Event EventRegistry::routeMessage(const keto::event::Event& event) {
    return RouterService::getInstance()->routeMessage(event);
}


keto::event::Event EventRegistry::updateStateRouteMessage(const keto::event::Event& event) {
    return RouterService::getInstance()->updateStateRouteMessage(event);
}


keto::event::Event EventRegistry::registerRpcPeerClient(const keto::event::Event& event) {
    return RouterService::getInstance()->registerRpcPeerClient(event);
}

keto::event::Event EventRegistry::registerRpcPeerServer(const keto::event::Event& event) {
    return RouterService::getInstance()->registerRpcPeerServer(event);
}

keto::event::Event EventRegistry::processPushRpcPeer(const keto::event::Event& event) {
    return RouterService::getInstance()->processPushRpcPeer(event);
}

keto::event::Event EventRegistry::deregisterRpcPeer(const keto::event::Event& event) {
    return RouterService::getInstance()->deregisterRpcPeer(event);
}

keto::event::Event EventRegistry::activateRpcPeer(const keto::event::Event& event) {
    return RouterService::getInstance()->activateRpcPeer(event);
}

keto::event::Event EventRegistry::registerService(const keto::event::Event& event) {
    return RouterService::getInstance()->registerService(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}

keto::event::Event EventRegistry::consensusSessionAccepted(const keto::event::Event& event) {
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::ACCEPTED);
    return event;
}



keto::event::Event EventRegistry::consensusProtocolCheck(const keto::event::Event& event) {

    return event;
}

keto::event::Event EventRegistry::consensusHeartbeat(const keto::event::Event& event) {

    return event;
}

keto::event::Event EventRegistry::electRouterPeer(const keto::event::Event& event) {
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPeerMessage>(event));

    electionPeerMessageProtoHelper.setPeer(
            PeerCache::getInstance()->electPeer(electionPeerMessageProtoHelper.getAccount()));

    return keto::server_common::toEvent<keto::proto::ElectionPeerMessage>(electionPeerMessageProtoHelper);
}

keto::event::Event EventRegistry::electRpcProcessPublish(const keto::event::Event& event) {
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));
    TangleServiceCache::getInstance()->publish(electionPublishTangleAccountProtoHelper);
    return event;
}

keto::event::Event EventRegistry::electRpcProcessConfirmation(const keto::event::Event& event) {
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));
    TangleServiceCache::getInstance()->confirmation(electionConfirmationHelper);
    return event;
}

keto::event::Event EventRegistry::getProducers(const keto::event::Event& event) {
    TangleServiceCache::getInstance()->confirmation(electionConfirmationHelper);
    return event;
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ROUTE_MESSAGE,
            &keto::router::EventRegistry::routeMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::UPDATE_STATUS_ROUTE_MESSSAGE,
            &keto::router::EventRegistry::updateStateRouteMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::REGISTER_SERVICE_MESSAGE,
            &keto::router::EventRegistry::registerService);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::REGISTER_RPC_PEER_CLIENT,
            &keto::router::EventRegistry::registerRpcPeerClient);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::REGISTER_RPC_PEER_SERVER,
            &keto::router::EventRegistry::registerRpcPeerServer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ROUTER_QUERY::PROCESS_PUSH_RPC_PEER,
            &keto::router::EventRegistry::processPushRpcPeer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::DEREGISTER_RPC_PEER,
            &keto::router::EventRegistry::deregisterRpcPeer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ACTIVATE_RPC_PEER,
            &keto::router::EventRegistry::activateRpcPeer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::ROUTER,
            &keto::router::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::ROUTER,
            &keto::router::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::ROUTER,
            &keto::router::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::ROUTER,
            &keto::router::EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::ROUTER,
            &keto::router::EventRegistry::consensusHeartbeat);

    keto::server_common::registerEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_ROUTER_PEER,
            &keto::router::EventRegistry::electRouterPeer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_PUBLISH,
            &keto::router::EventRegistry::electRpcProcessPublish);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_CONFIRMATION,
            &keto::router::EventRegistry::electRpcProcessConfirmation);

    keto::server_common::registerEventHandler (
            keto::server_common::Events::PRODUCER_QUERY::GET_PRODUCER,
            &keto::router::EventRegistry::getProducers);
}

void EventRegistry::deregisterEventHandlers() {

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::PRODUCER_QUERY::GET_PRODUCER);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_PUBLISH);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_CONFIRMATION);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_ROUTER_PEER);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::REGISTER_RPC_PEER_CLIENT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::REGISTER_RPC_PEER_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::DEREGISTER_RPC_PEER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ACTIVATE_RPC_PEER);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::ROUTER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::ROUTER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::ROUTER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::ROUTER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::ROUTER);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::REGISTER_SERVICE_MESSAGE);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::UPDATE_STATUS_ROUTE_MESSSAGE);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::ROUTE_MESSAGE);
}


}
}
