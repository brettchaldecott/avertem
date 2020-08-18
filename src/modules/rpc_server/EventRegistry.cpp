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

#include "keto/rpc_server/EventRegistry.hpp"
#include "keto/rpc_server/RpcServerService.hpp"
#include "keto/rpc_server/ConsensusService.hpp"
#include "keto/rpc_server/RpcServer.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"


namespace keto {
namespace rpc_server {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_SEND_MESSAGE,
            &keto::rpc_server::EventRegistry::sendMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::RPC_SERVER,
            &keto::rpc_server::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::RPC_SERVER,
            &keto::rpc_server::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::RPC_SERVER,
            &keto::rpc_server::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::RPC_SERVER,
            &keto::rpc_server::EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_SERVER,
            &keto::rpc_server::EventRegistry::consensusHeartbeat);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_SERVER_TRANSACTION,
            &keto::rpc_server::EventRegistry::routeTransaction);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_SERVER_BLOCK,
            &keto::rpc_server::EventRegistry::pushBlock);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_SERVER_REQUEST_BLOCK_SYNC,
            &keto::rpc_server::EventRegistry::requestBlockSync);

    keto::server_common::registerEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_SERVER,
            &keto::rpc_server::EventRegistry::electBlockProducer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_SERVER,
            &keto::rpc_server::EventRegistry::electRpcPublishServer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_SERVER,
            &keto::rpc_server::EventRegistry::electRpcConfirmationServer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::RPC_SERVER_ACTIVATE_RPC_PEER,
            &keto::rpc_server::EventRegistry::activatePeers);

    keto::server_common::registerEventHandler (
            keto::server_common::Events::REQUEST_NETWORK_STATE_SERVER,
            &keto::rpc_server::EventRegistry::requestNetworkState);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ACTIVATE_NETWORK_STATE_SERVER,
            &keto::rpc_server::EventRegistry::activateNetworkState);
}

void EventRegistry::deregisterEventHandlers() {

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ACTIVATE_NETWORK_STATE_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::REQUEST_NETWORK_STATE_SERVER);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_SERVER_ACTIVATE_RPC_PEER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_SERVER);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_SERVER_BLOCK);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_SERVER_TRANSACTION);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::RPC_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::RPC_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::RPC_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::RPC_SERVER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_SEND_MESSAGE);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::RPC_SERVER_REQUEST_BLOCK_SYNC);
}

keto::event::Event EventRegistry::sendMessage(const keto::event::Event& event) {
    return RpcServerService::getInstance()->sendMessage(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}

keto::event::Event EventRegistry::routeTransaction(const keto::event::Event& event) {
    return RpcServer::getInstance()->routeTransaction(event);
}

keto::event::Event EventRegistry::pushBlock(const keto::event::Event& event) {
    return RpcServer::getInstance()->pushBlock(event);
}

keto::event::Event EventRegistry::consensusSessionAccepted(const keto::event::Event& event) {
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::ACCEPTED);
    return RpcServer::getInstance()->performNetworkSessionReset(event);
}

keto::event::Event EventRegistry::consensusProtocolCheck(const keto::event::Event& event) {
    return RpcServer::getInstance()->performProtocoCheck(event);
}

keto::event::Event EventRegistry::consensusHeartbeat(const keto::event::Event& event) {
    return RpcServer::getInstance()->performConsensusHeartbeat(event);
}

keto::event::Event EventRegistry::electBlockProducer(const keto::event::Event& event) {
    return RpcServer::getInstance()->electBlockProducer(event);
}

keto::event::Event EventRegistry::electRpcPublishServer(const keto::event::Event& event) {
    return RpcServer::getInstance()->electBlockProducerPublish(event);
}

keto::event::Event EventRegistry::electRpcConfirmationServer(const keto::event::Event& event) {
    return RpcServer::getInstance()->electBlockProducerConfirmation(event);
}


keto::event::Event EventRegistry::activatePeers(const keto::event::Event& event) {
    return RpcServer::getInstance()->activatePeers(event);
}

keto::event::Event EventRegistry::requestNetworkState(const keto::event::Event& event) {
    return RpcServer::getInstance()->requestNetworkState(event);
}

keto::event::Event EventRegistry::activateNetworkState(const keto::event::Event& event) {
    return RpcServer::getInstance()->activateNetworkState(event);
}

keto::event::Event EventRegistry::requestBlockSync(const keto::event::Event& event) {
    return RpcServer::getInstance()->requestBlockSync(event);
}


}
}
