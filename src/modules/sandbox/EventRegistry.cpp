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

#include "keto/sandbox/EventRegistry.hpp"
#include "keto/sandbox/SandboxService.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/sandbox/SandboxService.hpp"
#include "keto/sandbox/ConsensusService.hpp"


namespace keto {
namespace sandbox {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::EXECUTE_ACTION_MESSAGE,
            &keto::sandbox::EventRegistry::executeActionMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::SANDBOX,
            &keto::sandbox::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::SANDBOX,
            &keto::sandbox::EventRegistry::setModuleSession);
}

void EventRegistry::deregisterEventHandlers() {
    
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::SANDBOX);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::SANDBOX);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::EXECUTE_ACTION_MESSAGE);
}

keto::event::Event EventRegistry::executeActionMessage(const keto::event::Event& event) {
    return SandboxService::getInstance()->executeActionMessage(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}


}
}
