//
// Created by Brett Chaldecott on 2019/01/14.
//

#include "keto/memory_vault_module/EventRegistry.hpp"


#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/memory_vault_module/ConsensusService.hpp"


namespace keto {
namespace memory_vault_module {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS::MEMORY_VAULT_MANAGER_QUERY,
            &keto::memory_vault_module::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::MEMORY_VAULT_MANAGER_QUERY,
            &keto::memory_vault_module::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::SETUP_NODE_CONSENSUS_SESSION,
            &EventRegistry::setupNodeConsensusSession);
}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SETUP_NODE_CONSENSUS_SESSION);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::MEMORY_VAULT_MANAGER_QUERY);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS::MEMORY_VAULT_MANAGER_QUERY);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event &event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event &event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}

keto::event::Event EventRegistry::setupNodeConsensusSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setupNodeConsensusSession(event);
}

}
}