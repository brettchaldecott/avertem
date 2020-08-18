//
// Created by Brett Chaldecott on 2019/01/14.
//


#include <boost/dll/alias.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/shared_ptr.hpp>

#include "keto/memory_vault_module/MemoryVaultModuleManager.hpp"
#include "keto/memory_vault_module/MemoryVaultModuleManagerMisc.hpp"
#include "keto/memory_vault_module/ConsensusService.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/common/Log.hpp"

#include "keto/memory_vault/MemoryVaultManager.hpp"

#include "keto/memory_vault_module/ConsensusService.hpp"
#include "keto/memory_vault_module/EventRegistry.hpp"

#include "keto/memory_vault_module/MemoryVaultModuleManager.hpp"


namespace keto {
namespace memory_vault_module {

std::string MemoryVaultModuleManager::getSourceVersion() {
return OBFUSCATED("$Id$");
}


MemoryVaultModuleManager::MemoryVaultModuleManager() {
}

MemoryVaultModuleManager::~MemoryVaultModuleManager() {
}

// meta methods
const std::string MemoryVaultModuleManager::getName() const {
    return "MemoryVaultModuleManager";
}
const std::string MemoryVaultModuleManager::getDescription() const {
    return "The memory vault";
}

const std::string MemoryVaultModuleManager::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}

// lifecycle methods
void MemoryVaultModuleManager::start() {
    KETO_LOG_INFO << "Start has been called on the memory vault module manager";
    ConsensusService::init(getConsensusHash());
    keto::memory_vault::MemoryVaultManager::init();
    modules["memory_vault_module"] = std::make_shared<MemoryVaultModule>();
    EventRegistry::registerEventHandlers();
}

void MemoryVaultModuleManager::stop() {
    KETO_LOG_INFO << "Stop has been called on the memory vault module manager";
    ConsensusService::fin();
    keto::memory_vault::MemoryVaultManager::fin();
    EventRegistry::deregisterEventHandlers();
    modules.clear();
    KETO_LOG_INFO << "The memory vault module has been stopped";

}

const std::vector<std::string> MemoryVaultModuleManager::listModules() {
    std::vector<std::string> keys;
    std::transform(
            this->modules.begin(),
            this->modules.end(),
            std::back_inserter(keys),
            [](const std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>>::value_type
                                                                                         &pair){return pair.first;});
    return keys;
}

const std::shared_ptr<keto::module::ModuleInterface>
MemoryVaultModuleManager::getModule(const std::string& name) {
    return modules[name];
}

boost::shared_ptr<MemoryVaultModuleManager> MemoryVaultModuleManager::create_module() {
    return boost::shared_ptr<MemoryVaultModuleManager>(new MemoryVaultModuleManager());
}

BOOST_DLL_ALIAS(
        keto::memory_vault_module::MemoryVaultModuleManager::create_module,
        create_module
)



}
}