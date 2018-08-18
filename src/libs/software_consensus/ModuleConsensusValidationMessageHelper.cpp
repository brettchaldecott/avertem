/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusValidationMessageHelper.cpp
 * Author: ubuntu
 * 
 * Created on August 17, 2018, 8:39 AM
 */

#include "keto/software_consensus/ModuleConsensusValidationMessageHelper.hpp"

namespace keto {
namespace software_consensus {

ModuleConsensusValidationMessageHelper::ModuleConsensusValidationMessageHelper() {
    moduleConsensusValidationMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ModuleConsensusValidationMessageHelper::ModuleConsensusValidationMessageHelper(
        const keto::proto::ModuleConsensusValidationMessage& moduleConsensusValidationMessage) :
    moduleConsensusValidationMessage(moduleConsensusValidationMessage) {
}

ModuleConsensusValidationMessageHelper::~ModuleConsensusValidationMessageHelper() {
}

ModuleConsensusValidationMessageHelper& ModuleConsensusValidationMessageHelper::setValid(bool valid) {
    moduleConsensusValidationMessage.set_valid(valid);
    return *this;
}

bool ModuleConsensusValidationMessageHelper::isValid() {
    return moduleConsensusValidationMessage.valid();
}

ModuleConsensusValidationMessageHelper::operator keto::proto::ModuleConsensusValidationMessage() {
    return this->moduleConsensusValidationMessage;
}


}
}

