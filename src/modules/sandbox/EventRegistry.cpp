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

#include "keto/software_consensus/ConsensusStateManager.hpp"


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
            keto::server_common::Events::EXECUTE_HTTP_CONTRACT_MESSAGE,
            &keto::sandbox::EventRegistry::executeHttpActionMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::SANDBOX,
            &keto::sandbox::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::SANDBOX,
            &keto::sandbox::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::SANDBOX,
            &keto::sandbox::EventRegistry::consensusSessionAccepted);
}

void EventRegistry::deregisterEventHandlers() {

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::SANDBOX);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::SANDBOX);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::SANDBOX);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::EXECUTE_ACTION_MESSAGE);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::EXECUTE_HTTP_CONTRACT_MESSAGE);
}

keto::event::Event EventRegistry::executeActionMessage(const keto::event::Event& event) {
    return SandboxService::getInstance()->executeActionMessage(event);
}

keto::event::Event EventRegistry::executeHttpActionMessage(const keto::event::Event& event) {
    return SandboxService::getInstance()->executeHttpActionMessage(event);
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
