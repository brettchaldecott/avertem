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

#include "keto/balancer/EventRegistry.hpp"
#include "keto/balancer/BalancerService.hpp"
#include "keto/balancer/ConsensusService.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"


namespace keto {
namespace balancer {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::BALANCER_MESSAGE,
            &keto::balancer::EventRegistry::balanceMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::BALANCER,
            &keto::balancer::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::BALANCER,
            &keto::balancer::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::BALANCER,
            &keto::balancer::EventRegistry::consensusSessionAccepted);
}

void EventRegistry::deregisterEventHandlers() {
    
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::BALANCER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::BALANCER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::BALANCER);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::BALANCER_MESSAGE);
}

keto::event::Event EventRegistry::balanceMessage(const keto::event::Event& event) {
    return BalancerService::getInstance()->balanceMessage(event);
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

}
}