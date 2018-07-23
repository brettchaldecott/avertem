/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusServices.cpp
 * Author: ubuntu
 * 
 * Created on July 18, 2018, 12:42 PM
 */

#include <condition_variable>

#include "SoftwareConsensus.pb.h"
#include "HandShake.pb.h"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/event/Event.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/software_consensus/ModuleSessionMessageHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ConsensusBuilder.hpp"

#include "keto/consensus_module/ConsensusServices.hpp"

#include "keto/consensus_module/Exception.hpp"
#include "keto/consensus_module/Constants.hpp"
#include "include/keto/consensus_module/ConsensusServices.hpp"

namespace keto {
namespace consensus_module {

static ConsensusServicesPtr singleton;

ConsensusServices::ConsensusServices(
            const keto::software_consensus::ConsensusHashGeneratorPtr& seedHashGenerator,
            const keto::software_consensus::ConsensusHashGeneratorPtr& moduleHashGenerator) :
    seedHashGenerator(seedHashGenerator),
    moduleHashGenerator(moduleHashGenerator) {
    
    // setup the key loader
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::consensus_module::PrivateKeyNotConfiguredException());
    }
    std::string privateKeyPath = 
            config->getVariablesMap()[Constants::PRIVATE_KEY].as<std::string>();
    if (!config->getVariablesMap().count(Constants::PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::consensus_module::PrivateKeyNotConfiguredException());
    }
    std::string publicKeyPath = 
            config->getVariablesMap()[Constants::PUBLIC_KEY].as<std::string>();
    this->keyLoaderPtr = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
            publicKeyPath);
    
}

ConsensusServices::~ConsensusServices() {
}

// account service management methods
ConsensusServicesPtr ConsensusServices::init(
        const keto::software_consensus::ConsensusHashGeneratorPtr& seedHashGenerator,
        const keto::software_consensus::ConsensusHashGeneratorPtr& moduleHashGenerator)  {
    return singleton = std::shared_ptr<ConsensusServices>(
            new ConsensusServices(seedHashGenerator,moduleHashGenerator));
}

void ConsensusServices::fin() {
    if (singleton) {
        singleton.reset();
    }
}

ConsensusServicesPtr ConsensusServices::getInstance() {
    return singleton;
}

keto::event::Event ConsensusServices::generateSoftwareConsensus(const keto::event::Event& event) {
    keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper(
        keto::server_common::fromEvent<keto::proto::ModuleHashMessage>(event));
    keto::proto::ConsensusMessage result = keto::software_consensus::ConsensusBuilder(
            this->seedHashGenerator,
            this->keyLoaderPtr).
            buildConsensus(moduleHashMessageHelper.getHash())
            .getConsensus().operator keto::proto::ConsensusMessage();
    return keto::server_common::toEvent<keto::proto::ConsensusMessage>(result);
}

keto::event::Event ConsensusServices::generateSoftwareHash(const keto::event::Event& event) {
    keto::software_consensus::ModuleConsensusHelper moduleConsensusHelper(
        keto::server_common::fromEvent<keto::proto::ModuleConsensusMessage>(event));
    moduleConsensusHelper.setModuleHash(this->moduleHashGenerator->generateHash(
            moduleConsensusHelper.getSeedHash().operator keto::crypto::SecureVector()));
    keto::proto::ModuleConsensusMessage moduleConsensusMessage =
            moduleConsensusHelper.getModuleConsensusMessage();
    return keto::server_common::toEvent<keto::proto::ModuleConsensusMessage>(moduleConsensusMessage);
}

keto::event::Event ConsensusServices::setModuleSession(const keto::event::Event& event) {
    keto::software_consensus::ModuleSessionMessageHelper moduleSessionHelper(
        keto::server_common::fromEvent<keto::proto::ModuleSessionMessage>(event));
    this->moduleHashGenerator->setSession(moduleSessionHelper.getSecret());
    this->seedHashGenerator->setSession(moduleSessionHelper.getSecret());
    return event;
}

}
}