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

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/consensus_module/EventRegistry.hpp"
#include "keto/consensus_module/ConsensusServices.hpp"
#include "keto/software_consensus/ConsensusStateManager.hpp"

namespace keto {
namespace consensus_module {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,
            &keto::consensus_module::EventRegistry::generateSoftwareConsensus);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::VALIDATE_SOFTWARE_CONSENSUS_MESSAGE,
            &keto::consensus_module::EventRegistry::validateSoftwareConsensus);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::consensusHeartbeat);
}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::VALIDATE_SOFTWARE_CONSENSUS_MESSAGE);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::CONSENSUS_QUERY);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::CONSENSUS_QUERY);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::CONSENSUS_QUERY);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::CONSENSUS_QUERY);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::CONSENSUS_QUERY);
}

keto::event::Event EventRegistry::generateSoftwareConsensus(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->generateSoftwareConsensus(event);
}

keto::event::Event EventRegistry::validateSoftwareConsensus(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->validateSoftwareConsensus(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->setModuleSession(event);
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



}
}
