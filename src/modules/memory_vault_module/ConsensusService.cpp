//
// Created by Brett Chaldecott on 2019/01/14.
//

#include "keto/memory_vault_module/ConsensusService.hpp"

#include <condition_variable>

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/memory_vault/MemoryVaultManager.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"
#include "keto/software_consensus/ConsensusMessageHelper.hpp"
#include "keto/software_consensus/ModuleSessionMessageHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"


namespace keto {
namespace memory_vault_module {

static ConsensusServicePtr singleton;

std::string ConsensusService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


ConsensusService::ConsensusService(
        const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator) :
        consensusHashGenerator(consensusHashGenerator) {
    keto::software_consensus::ConsensusStateManager::init();
}

ConsensusService::~ConsensusService() {
    keto::software_consensus::ConsensusStateManager::fin();
}


// account service management methods
ConsensusServicePtr ConsensusService::init(
        const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator) {
    return singleton = ConsensusServicePtr(new ConsensusService(consensusHashGenerator));
}

void ConsensusService::fin() {
    singleton.reset();
}

ConsensusServicePtr ConsensusService::getInstance() {
    return singleton;
}

keto::event::Event ConsensusService::generateSoftwareHash(const keto::event::Event& event) {
    keto::software_consensus::ModuleConsensusHelper moduleConsensusHelper(
            keto::server_common::fromEvent<keto::proto::ModuleConsensusMessage>(event));
    moduleConsensusHelper.setModuleHash(this->consensusHashGenerator->generateHash(
            moduleConsensusHelper.getSeedHash().operator keto::crypto::SecureVector()));
    keto::proto::ModuleConsensusMessage moduleConsensusMessage =
            moduleConsensusHelper.getModuleConsensusMessage();
    return keto::server_common::toEvent<keto::proto::ModuleConsensusMessage>(moduleConsensusMessage);
}

keto::event::Event ConsensusService::setModuleSession(const keto::event::Event& event) {
    keto::software_consensus::ModuleSessionMessageHelper moduleSessionHelper(
            keto::server_common::fromEvent<keto::proto::ModuleSessionMessage>(event));
    //std::cout << "Setup the memory module session key" << std::endl;
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::GENERATE);
    this->consensusHashGenerator->setSession(moduleSessionHelper.getSecret());
    keto::memory_vault::MemoryVaultManager::getInstance()->clearSession();
    //std::cout << "Clear the sessions" << std::endl;
    return event;
}

keto::event::Event ConsensusService::setupNodeConsensusSession(const keto::event::Event& event) {
    keto::software_consensus::ConsensusMessageHelper consensusMessageHelper(
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(event));
    keto::memory_vault::vectorOfSecureVectors vectorOfSecureVectors;
    for (keto::asn1::HashHelper hashHelper : consensusMessageHelper.getMsg().getSystemHashes()) {
        vectorOfSecureVectors.push_back((keto::crypto::SecureVector)hashHelper);
    }
    keto::memory_vault::MemoryVaultManager::getInstance()->createSession(vectorOfSecureVectors);
    return event;
}


}
}