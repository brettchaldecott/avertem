/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcClientModuleManager.cpp
 * Author: ubuntu
 * 
 * Created on January 20, 2018, 1:40 PM
 */

#include <boost/dll/alias.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/shared_ptr.hpp>

#include "keto/common/Log.hpp"

#include "keto/rpc_client/RpcClientModuleManager.hpp"
#include "keto/rpc_client/RpcClientModuleManagerMisc.hpp"
#include "keto/rpc_client/RpcSessionManager.hpp"
#include "keto/rpc_client/EventRegistry.hpp"
#include "keto/common/MetaInfo.hpp"
#include "include/keto/rpc_client/ConsensusService.hpp"


namespace keto {
namespace rpc_client {

std::string RpcClientModuleManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcClientModuleManager::RpcClientModuleManager() {
}

RpcClientModuleManager::~RpcClientModuleManager() {
}

// meta methods
const std::string RpcClientModuleManager::getName() const {
    return "RpcClientModuleManager";
}

const std::string RpcClientModuleManager::getDescription() const {
    return "The rpc client module manager responsible for managing out going connections";
}

const std::string RpcClientModuleManager::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}

// lifecycle methods
void RpcClientModuleManager::start() {
    modules["RpcClientModule"] = std::make_shared<RpcClientModule>();
    ConsensusService::init(getConsensusHash());
    EventRegistry::registerEventHandlers();
    RpcSessionManager::init();
    RpcSessionManager::getInstance()->start();
    KETO_LOG_INFO << "[RpcClientModuleManager] Started the RpcClientModuleManager";
}

void RpcClientModuleManager::postStart() {
    RpcSessionManager::getInstance()->postStart();
    KETO_LOG_INFO << "[RpcClientModuleManager] Post Started the RpcClientModuleManager";
}

void RpcClientModuleManager::stop() {
    EventRegistry::deregisterEventHandlers();
    ConsensusService::fin();
    modules.clear();
    RpcSessionManager::getInstance()->stop();
    RpcSessionManager::fin();
    KETO_LOG_INFO << "[RpcClientModuleManager] The RpcClientModuleManager is being stopped";
}

const std::vector<std::string> RpcClientModuleManager::listModules() {
    std::vector<std::string> keys;
    std::transform(
        this->modules.begin(),
        this->modules.end(),
        std::back_inserter(keys),
        [](const std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>>::value_type 
            &pair){return pair.first;});
    return keys;
}

const std::shared_ptr<keto::module::ModuleInterface> RpcClientModuleManager::getModule(const std::string& name) {
    return modules[name];
}

boost::shared_ptr<RpcClientModuleManager> RpcClientModuleManager::create_module() {
    return boost::shared_ptr<RpcClientModuleManager>(new RpcClientModuleManager());
}


BOOST_DLL_ALIAS(
    keto::rpc_client::RpcClientModuleManager::create_module,
    create_module                               
)

}
}