//
// Created by Brett Chaldecott on 2019/01/14.
//

#include "keto/memory_vault_module/EventRegistry.hpp"


#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"

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
            keto::server_common::Events::CONSENSUS::MEMORY_VAULT_MANAGER,
            &keto::memory_vault_module::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::MEMORY_VAULT_MANAGER,
            &keto::memory_vault_module::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::MEMORY_VAULT_MANAGER,
            &keto::memory_vault_module::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::MEMORY_VAULT_MANAGER,
            &keto::memory_vault_module::EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_HEARTBEAT::MEMORY_VAULT_MANAGER,
            &keto::memory_vault_module::EventRegistry::consensusHeartbeat);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_STATE::MEMORY_VAULT_MANAGER,
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
            keto::server_common::Events::CONSENSUS_SESSION_STATE::MEMORY_VAULT_MANAGER);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_HEARTBEAT::MEMORY_VAULT_MANAGER);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::MEMORY_VAULT_MANAGER);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::MEMORY_VAULT_MANAGER);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::MEMORY_VAULT_MANAGER);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS::MEMORY_VAULT_MANAGER);
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
