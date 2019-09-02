/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.cpp
 * Author: ubuntu
 * 
 * Created on March 8, 2018, 3:15 AM
 */

#include "keto/rpc_client/EventRegistry.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/rpc_client/ConsensusService.hpp"
#include "keto/rpc_client/RpcSessionManager.hpp"


namespace keto {
namespace rpc_client {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}

keto::event::Event EventRegistry::routeTransaction(const keto::event::Event& event) {
    return RpcSessionManager::getInstance()->routeTransaction(event);
}

keto::event::Event EventRegistry::activatePeer(const keto::event::Event& event) {
    return RpcSessionManager::getInstance()->activatePeer(event);
}

keto::event::Event EventRegistry::requestBlockSync(const keto::event::Event& event) {
    return RpcSessionManager::getInstance()->requestBlockSync(event);
}

keto::event::Event EventRegistry::pushBlock(const keto::event::Event& event) {
    return RpcSessionManager::getInstance()->pushBlock(event);
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

keto::event::Event EventRegistry::electBlockProducer(const keto::event::Event& event) {
    return RpcSessionManager::getInstance()->electBlockProducer(event);
}

void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::RPC_CLIENT,
            &keto::rpc_client::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::RPC_CLIENT,
            &keto::rpc_client::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::RPC_CLIENT,
            &keto::rpc_client::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::RPC_CLIENT,
            &keto::rpc_client::EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_CLIENT,
            &keto::rpc_client::EventRegistry::consensusHeartbeat);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_CLIENT_TRANSACTION,
            &keto::rpc_client::EventRegistry::routeTransaction);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_CLIENT_REQUEST_BLOCK_SYNC,
            &keto::rpc_client::EventRegistry::requestBlockSync);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_CLIENT_BLOCK,
            &keto::rpc_client::EventRegistry::pushBlock);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_CLIENT_ACTIVATE_RPC_PEER,
            &keto::rpc_client::EventRegistry::activatePeer);

    // election methods
    keto::server_common::registerEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CLIENT,
            &keto::rpc_client::EventRegistry::electBlockProducer);


}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_CLIENT_ACTIVATE_RPC_PEER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_CLIENT_BLOCK);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_CLIENT_REQUEST_BLOCK_SYNC);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_CLIENT_TRANSACTION);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_CLIENT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::RPC_CLIENT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::RPC_CLIENT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::RPC_CLIENT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::RPC_CLIENT);

    // election methods
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CLIENT);
}

}
}
