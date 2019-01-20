//
// Created by Brett Chaldecott on 2019/01/14.
//

#include "keto/memory_vault_module/EventRegistry.hpp"


#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/memory_vault_module/ConsensusService.hpp"
#include "keto/memory_vault_module/MemoryVaultModuleService.hpp"


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

    keto::server_common::registerEventHandler(
            keto::server_common::Events::MEMORY_VAULT::CREATE_VAULT,
            &keto::memory_vault_module::EventRegistry::createVault);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::MEMORY_VAULT::ADD_ENTRY,
            &keto::memory_vault_module::EventRegistry::addEntry);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::MEMORY_VAULT::SET_ENTRY,
            &keto::memory_vault_module::EventRegistry::setEntry);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::MEMORY_VAULT::GET_ENTRY,
            &keto::memory_vault_module::EventRegistry::getEntry);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::MEMORY_VAULT::REMOVE_ENTRY,
            &keto::memory_vault_module::EventRegistry::removeEntry);
}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::MEMORY_VAULT::CREATE_VAULT);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::MEMORY_VAULT::ADD_ENTRY);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::MEMORY_VAULT::SET_ENTRY);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::MEMORY_VAULT::GET_ENTRY);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::MEMORY_VAULT::REMOVE_ENTRY);

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


keto::event::Event EventRegistry::createVault(const keto::event::Event &event) {
    return MemoryVaultModuleService::getInstance()->createVault(event);
}

keto::event::Event EventRegistry::addEntry(const keto::event::Event &event) {
    return MemoryVaultModuleService::getInstance()->addEntry(event);
}

keto::event::Event EventRegistry::setEntry(const keto::event::Event &event) {
    return MemoryVaultModuleService::getInstance()->setEntry(event);
}


keto::event::Event EventRegistry::getEntry(const keto::event::Event &event) {
    return MemoryVaultModuleService::getInstance()->getEntry(event);
}

keto::event::Event EventRegistry::removeEntry(const keto::event::Event &event) {
    return MemoryVaultModuleService::getInstance()->removeEntry(event);
}

}
}