/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleHashMessageHelper.cpp
 * Author: ubuntu
 * 
 * Created on July 19, 2018, 6:30 AM
 */


#include "SoftwareConsensus.pb.h"

#include "keto/common/MetaInfo.hpp"

#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ModuleHashMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ModuleHashMessageHelper::ModuleHashMessageHelper() {
    modulesHashMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ModuleHashMessageHelper::ModuleHashMessageHelper(
            const keto::proto::ModuleHashMessage& modulesHashMessage) : 
    modulesHashMessage(modulesHashMessage) {
    
}

ModuleHashMessageHelper::~ModuleHashMessageHelper() {
}

ModuleHashMessageHelper& ModuleHashMessageHelper::setHash(
        const keto::crypto::SecureVector& hash) {
    modulesHashMessage.set_hash(keto::crypto::SecureVectorUtils().copySecureToString(hash));
    return *this;
}

keto::asn1::HashHelper ModuleHashMessageHelper::getHash() {
    return keto::asn1::HashHelper(keto::crypto::SecureVectorUtils().copyStringToSecure(
            modulesHashMessage.hash()));
}

keto::crypto::SecureVector ModuleHashMessageHelper::getHash_lock() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            modulesHashMessage.hash());
}

ModuleHashMessageHelper::operator keto::proto::ModuleHashMessage() {
    return this->modulesHashMessage;    
}

keto::proto::ModuleHashMessage ModuleHashMessageHelper::getModuleHashMessage() {
    return this->modulesHashMessage;
}

}
}