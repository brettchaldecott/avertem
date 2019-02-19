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


#include "keto/block/ConsensusService.hpp"
#include "keto/block/BlockProducer.hpp"
#include "keto/block/NetworkFeeManager.hpp"


namespace keto{
namespace block {
    
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
    NetworkFeeManager::getInstance()->clear();
    return event;
}

keto::event::Event ConsensusService::setupNodeConsensusSession(const keto::event::Event& event) {
    BlockProducer::getInstance()->setupNodeConsensusSession(event);
    return event;
}

keto::event::Event ConsensusService::consensusSessionAccepted(const keto::event::Event& event) {
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::ACCEPTED);
    if (keto::server_common::ServerInfo::getInstance()->isMaster() &&
        BlockProducer::getInstance()->getState() == BlockProducer::State::inited) {
        BlockProducer::getInstance()->setState(BlockProducer::State::block_producer);
        NetworkFeeManager::getInstance()->load();
    }

    return event;
}

}
}
