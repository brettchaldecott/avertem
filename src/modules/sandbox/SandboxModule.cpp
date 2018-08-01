/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SandboxModule.cpp
 * Author: ubuntu
 * 
 * Created on January 20, 2018, 4:29 PM
 */

#include "keto/sandbox/SandboxModule.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto{
namespace sandbox {
    
std::string SandboxModule::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}


SandboxModule::SandboxModule() {
}

SandboxModule::~SandboxModule() {
}

// meta methods
const std::string SandboxModule::getName() const {
    return "SandboxModule";
}

const std::string SandboxModule::getDescription() const {
    return "The Sandbox Module responsible for processing securely all transactions.";
}

const std::string SandboxModule::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}


}
}
