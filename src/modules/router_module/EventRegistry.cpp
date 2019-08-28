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

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"


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


keto::event::Event EventRegistry::registerRpcPeer(const keto::event::Event& event) {
    return RouterService::getInstance()->registerRpcPeer(event);
}

keto::event::Event EventRegistry::deregisterRpcPeer(const keto::event::Event& event) {
    return RouterService::getInstance()->deregisterRpcPeer(event);
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
            keto::server_common::Events::REGISTER_RPC_PEER,
            &keto::router::EventRegistry::registerRpcPeer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::DEREGISTER_RPC_PEER,
            &keto::router::EventRegistry::deregisterRpcPeer);
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

}

void EventRegistry::deregisterEventHandlers() {

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ROUTER_QUERY::ELECT_ROUTER_PEER);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::REGISTER_RPC_PEER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::DEREGISTER_RPC_PEER);

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
