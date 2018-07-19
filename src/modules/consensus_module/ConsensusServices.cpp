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

#include "keto/consensus_module/ConsensusServices.hpp"

namespace keto {
namespace consensus_module {

static ConsensusServicesPtr singleton;

ConsensusServices::ConsensusServices(
            const keto::software_consensus::ConsensusHashGeneratorPtr& seedHashGenerator,
            const keto::software_consensus::ConsensusHashGeneratorPtr& moduleHashGenerator) :
    seedHashGenerator(seedHashGenerator),
    moduleHashGenerator(moduleHashGenerator) {
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
    
}

keto::event::Event ConsensusServices::generateSoftwareHash(const keto::event::Event& event) {
    
}

keto::event::Event ConsensusServices::setModuleSession(const keto::event::Event& event) {
    
}

}
}