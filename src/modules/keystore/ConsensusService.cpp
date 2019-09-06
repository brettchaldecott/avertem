/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusService.cpp
 * Author: ubuntu
 * 
 * Created on July 23, 2018, 11:35 AM
 */

#include <condition_variable>

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"


#include "keto/software_consensus/ConsensusStateManager.hpp"
#include "keto/software_consensus/ModuleSessionMessageHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"

#include "keto/keystore/ConsensusService.hpp"
#include "keto/memory_vault_session/MemoryVaultSession.hpp"

#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/keystore/MasterKeyManager.hpp"

namespace keto{
namespace keystore {
    
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
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::GENERATE);
    this->consensusHashGenerator->setSession(moduleSessionHelper.getSecret());
    MasterKeyManager::getInstance()->clearSession();
    keto::memory_vault_session::MemoryVaultSession::getInstance()->clearSession();
    NetworkSessionKeyManager::getInstance()->clearSession();
    return event;
}



keto::event::Event ConsensusService::consensusSessionAccepted(const keto::event::Event& event) {
    //std::cout << "[consensusSessionAccepted] consensus session accepted" << std::endl;
    keto::memory_vault_session::MemoryVaultSession::getInstance()->initSession();
    NetworkSessionKeyManager::getInstance()->generateSession();
    //std::cout << "[consensusSessionAccepted] init the store" << std::endl;
    MasterKeyManager::getInstance()->initSession();
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::ACCEPTED);
    //std::cout << "[consensusSessionAccepted] return the event" << std::endl;
    return event;
}


}
}
