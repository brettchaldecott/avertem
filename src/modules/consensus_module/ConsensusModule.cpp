/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusModule.cpp
 * Author: ubuntu
 * 
 * Created on July 18, 2018, 7:16 AM
 */

#include "keto/consensus_module/ConsensusModule.hpp"

namespace keto {
namespace consensus_module {

std::string ConsensusModule::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ConsensusModule::ConsensusModule() {
}

ConsensusModule::~ConsensusModule() {
}

const std::string ConsensusModule::getName() const {
    return "ConsensusModule";
}

const std::string ConsensusModule::getDescription() const {
    return "The consensus module responsible for generating the software consensus.";
}

const std::string ConsensusModule::getVersion() const {
    return "0.1.0";
}


}
}