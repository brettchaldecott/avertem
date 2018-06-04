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

namespace keto {
namespace software_consensus {

ModuleConsensusGenerator::ModuleConsensusGenerator(const keto::proto::SoftwareConsensusMessage& 
            softwareConsensusMessage) : softwareConsensusMessage(softwareConsensusMessage) {
}

ModuleConsensusGenerator::~ModuleConsensusGenerator() {
}

keto::proto::SoftwareConsensusMessage ModuleConsensusGenerator::generate(
            const ModuleConsensusInterface& moduleConsensusInterface) {
    
}

}
}
