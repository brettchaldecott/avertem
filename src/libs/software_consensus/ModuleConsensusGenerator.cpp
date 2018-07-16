/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusGenerator.cpp
 * Author: ubuntu
 * 
 * Created on May 31, 2018, 10:14 AM
 */

#include "keto/software_consensus/ModuleConsensusGenerator.hpp"
#include "include/keto/software_consensus/ModuleConsensusHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ModuleConsensusGenerator::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}
    
ModuleConsensusGenerator::ModuleConsensusGenerator(
            const ConsensusHashGeneratorPtr& consensusHashGeneratorPtr,
            const keto::proto::ModuleConsensusMessage& moduleConsensusMessage) 
    : consensusHashGeneratorPtr(consensusHashGeneratorPtr),
        moduleConsensusHelper(moduleConsensusMessage) {
}

ModuleConsensusGenerator::~ModuleConsensusGenerator() {
    
}

keto::proto::ModuleConsensusMessage ModuleConsensusGenerator::generate() {
    this->moduleConsensusHelper.setModuleHash(
            consensusHashGeneratorPtr->generateHash(this->moduleConsensusHelper.getModuleHash_lock()));
    return this->moduleConsensusHelper.getModuleConsensusMessage();
}

}
}
