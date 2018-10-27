/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusHashScriptInfo.cpp
 * Author: ubuntu
 * 
 * Created on June 29, 2018, 11:07 AM
 */

#include "keto/software_consensus/ConsensusHashScriptInfo.hpp"

namespace keto {
namespace software_consensus {

std::string ConsensusHashScriptInfo::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
ConsensusHashScriptInfo::ConsensusHashScriptInfo(
        keto::software_consensus::getHash hashFunctionRef,
        keto::software_consensus::getEncodedKey keyFunctionRef,
        keto::software_consensus::getCode codeFunctionRef) : 
        hashFunctionRef(hashFunctionRef), keyFunctionRef(keyFunctionRef),
        codeFunctionRef(codeFunctionRef) {
}

ConsensusHashScriptInfo::~ConsensusHashScriptInfo() {
}


keto::crypto::SecureVector ConsensusHashScriptInfo::getHash() {
    return this->hashFunctionRef();
}

keto::crypto::SecureVector ConsensusHashScriptInfo::getEncodedKey() {
    return this->keyFunctionRef();
}

std::vector<uint8_t> ConsensusHashScriptInfo::getCode() {
    return this->codeFunctionRef();
}

}
}
