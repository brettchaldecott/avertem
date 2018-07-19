/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleSessionMessageHelper.cpp
 * Author: ubuntu
 * 
 * Created on July 19, 2018, 6:30 AM
 */

#include "keto/common/MetaInfo.hpp"

#include "keto/software_consensus/ModuleSessionMessageHelper.hpp"
#include "include/keto/software_consensus/ModuleSessionMessageHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ModuleSessionMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id: cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $");
}

ModuleSessionMessageHelper::ModuleSessionMessageHelper() {
    modulesSessionMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ModuleSessionMessageHelper::ModuleSessionMessageHelper(
            const keto::proto::ModuleSessionMessage& modulesSessionMessage) : 
    modulesSessionMessage(modulesSessionMessage) {
    
}

ModuleSessionMessageHelper::~ModuleSessionMessageHelper() {
}

ModuleSessionMessageHelper& ModuleSessionMessageHelper::setSecret(
        const keto::crypto::SecureVector& secret) {
    modulesSessionMessage.set_secret(keto::crypto::SecureVectorUtils().copySecureToString(secret));
    return *this;
}

keto::crypto::SecureVector ModuleSessionMessageHelper::getSecret() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            modulesSessionMessage.secret());
}

ModuleSessionMessageHelper::operator keto::proto::ModuleSessionMessage() {
    return this->modulesSessionMessage;
}

keto::proto::ModuleSessionMessage ModuleSessionMessageHelper::getModuleSessionMessage() {
    return this->modulesSessionMessage;
}

}
}