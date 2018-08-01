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


namespace keto {
namespace consensus_module {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id:$");
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
            keto::server_common::Events::CONSENSUS::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::CONSENSUS_QUERY,
            &keto::consensus_module::EventRegistry::setModuleSession);
}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::CONSENSUS_QUERY);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::CONSENSUS_QUERY);
}

keto::event::Event EventRegistry::generateSoftwareConsensus(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->generateSoftwareConsensus(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusServices::getInstance()->setModuleSession(event);
}


}
}