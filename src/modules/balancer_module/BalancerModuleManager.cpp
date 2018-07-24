/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BalancerModuleManager.cpp
 * Author: Brett Chaldecott
 * 
 * Created on February 10, 2018, 1:37 PM
 */

#include <boost/dll/alias.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/shared_ptr.hpp>

#include "keto/common/Log.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/balancer/BalancerModuleManager.hpp"
#include "keto/balancer/BalancerModuleManagerMisc.hpp"
#include "keto/balancer/BalancerService.hpp"
#include "keto/balancer/EventRegistry.hpp"
#include "keto/balancer/BlockRouting.hpp"
#include "keto/balancer/TransactionCache.hpp"
#include "keto/balancer/ConsensusService.hpp"
#include "keto/server_common/ServiceRegistryHelper.hpp"
#include "keto/server_common/Constants.hpp"
#include "include/keto/balancer/BlockRouting.hpp"
#include "include/keto/balancer/TransactionCache.hpp"
#include "include/keto/balancer/ConsensusService.hpp"

namespace keto {
namespace balancer {


BalancerModuleManager::BalancerModuleManager() {
}

BalancerModuleManager::~BalancerModuleManager() {
}

// meta methods
const std::string BalancerModuleManager::getName() const {
    return "BalancerModuleManager";
}

const std::string BalancerModuleManager::getDescription() const {
    return "The balancer module manager responsible for managing the balancer and filter.";
}

const std::string BalancerModuleManager::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}

// lifecycle methods
void BalancerModuleManager::start() {
    KETO_LOG_INFO << "[BalancerModuleManager] Starting the BalancerModuleManager";
    modules["balancerModule"] = std::make_shared<BalancerModule>();
    BalancerService::init();
    BlockRouting::init();
    TransactionCache::init();
    ConsensusService::init(getConsensusHash());
    EventRegistry::registerEventHandlers();
    KETO_LOG_INFO << "[BalancerModuleManager] Started the BalancerModuleManager";
}

void BalancerModuleManager::stop() {
    EventRegistry::deregisterEventHandlers();
    ConsensusService::fin();
    TransactionCache::fin();
    BlockRouting::fin();
    BalancerService::fin();
    modules.clear();
    KETO_LOG_INFO << "[BalancerModuleManager] The BalancerModuleManager is being stopped";
}


void BalancerModuleManager::postStart() {
    KETO_LOG_INFO << "[BalancerModuleManager] The postStart method has been called";
    keto::server_common::registerService(keto::server_common::Constants::SERVICE::BALANCE);
    KETO_LOG_INFO << "[BalancerModuleManager] The postStart is complete";
    
}

const std::vector<std::string> BalancerModuleManager::listModules() {
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
BalancerModuleManager::getModule(const std::string& name) {
    return modules[name];
}

boost::shared_ptr<BalancerModuleManager> BalancerModuleManager::create_module() {
    return boost::shared_ptr<BalancerModuleManager>(new BalancerModuleManager());    
}

BOOST_DLL_ALIAS(
    keto::balancer::BalancerModuleManager::create_module,
    create_module
)


}
}