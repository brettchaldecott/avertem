/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusHelper.cpp
 * Author: ubuntu
 * 
 * Created on June 16, 2018, 4:07 PM
 */

#include "keto/common/MetaInfo.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"


namespace keto {
namespace software_consensus {


std::string ModuleConsensusHelper::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

    
ModuleConsensusHelper::ModuleConsensusHelper() {
    moduleConsensusMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}


ModuleConsensusHelper::ModuleConsensusHelper(
    const keto::proto::ModuleConsensusMessage& moduleConsensusMessage) : 
    moduleConsensusMessage(moduleConsensusMessage) {
    
}


ModuleConsensusHelper::~ModuleConsensusHelper() {
    
}

ModuleConsensusHelper& ModuleConsensusHelper::setSeedHash(const std::vector<uint8_t>& seedHash) {
    this->moduleConsensusMessage.set_seed_hash(keto::server_common::VectorUtils().copyVectorToString(seedHash));
    return *this;
}

ModuleConsensusHelper& ModuleConsensusHelper::setSeedHash(const keto::asn1::HashHelper& hashHelper) {
    this->moduleConsensusMessage.set_seed_hash(
            keto::crypto::SecureVectorUtils().copySecureToString(
                hashHelper.operator keto::crypto::SecureVector()));
    return *this;
}

keto::asn1::HashHelper ModuleConsensusHelper::getSeedHash() {
    return keto::asn1::HashHelper(
            keto::crypto::SecureVectorUtils().copyStringToSecure(
            this->moduleConsensusMessage.seed_hash()));
}

ModuleConsensusHelper& ModuleConsensusHelper::setModuleHash(const std::vector<uint8_t>& moduleHash) {
    this->moduleConsensusMessage.set_module_hash(keto::server_common::VectorUtils().copyVectorToString(moduleHash));
    return *this;
}

ModuleConsensusHelper& ModuleConsensusHelper::setModuleHash(const keto::crypto::SecureVector& moduleHash) {
    this->moduleConsensusMessage.set_module_hash(keto::crypto::SecureVectorUtils().copySecureToString(moduleHash));
    return *this; 
}

ModuleConsensusHelper& ModuleConsensusHelper::setModuleHash(const keto::asn1::HashHelper& hashHelper) {
    this->moduleConsensusMessage.set_module_hash(
            keto::crypto::SecureVectorUtils().copySecureToString(
                hashHelper.operator keto::crypto::SecureVector()));
    return *this;
}

keto::asn1::HashHelper ModuleConsensusHelper::getModuleHash() {
    return keto::asn1::HashHelper(
            keto::crypto::SecureVectorUtils().copyStringToSecure(
            this->moduleConsensusMessage.module_hash()));
}

keto::crypto::SecureVector ModuleConsensusHelper::getModuleHash_lock() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            this->moduleConsensusMessage.module_hash());
}

ModuleConsensusHelper::operator keto::proto::ModuleConsensusMessage() {
    return this->moduleConsensusMessage;
}

keto::proto::ModuleConsensusMessage ModuleConsensusHelper::getModuleConsensusMessage() {
    return this->moduleConsensusMessage;
}

}
}