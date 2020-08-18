//
// Created by Brett Chaldecott on 2019/01/14.
//

#include "keto/memory_vault_module/MemoryVaultModule.hpp"
#include "keto/common/Log.hpp"
#include "keto/common/MetaInfo.hpp"



namespace keto {
namespace memory_vault_module {

std::string MemoryVaultModule::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


MemoryVaultModule::MemoryVaultModule() {
}

MemoryVaultModule::~MemoryVaultModule() {
}


const std::string MemoryVaultModule::getName() const {
    return "MemoryVaultModule";
}

const std::string MemoryVaultModule::getDescription() const {
    return "The memory vault module";
}

const std::string MemoryVaultModule::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}


}
}
