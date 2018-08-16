/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusModuleManager.cpp
 * Author: ubuntu
 * 
 * Created on July 18, 2018, 7:15 AM
 */

#include <boost/dll/alias.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/shared_ptr.hpp>

#include "keto/common/Log.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/consensus_module/ConsensusModuleManager.hpp"
#include "keto/consensus_module/ConsensusModuleManagerMisc.hpp"
#include "keto/consensus_module/ConsensusServices.hpp"
#include "include/keto/consensus_module/ConsensusServices.hpp"
#include "include/keto/consensus_module/EventRegistry.hpp"
#include "include/keto/consensus_module/ConsensusServer.hpp"

namespace keto {
namespace consensus_module {

std::string ConsensusModuleManager::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

ConsensusModuleManager::ConsensusModuleManager() {
    consensusServerPtr = std::make_shared<ConsensusServer>();
}

ConsensusModuleManager::~ConsensusModuleManager() {
}

// meta methods
const std::string ConsensusModuleManager::getName() const {
    return "ConsensusModuleManager";
}

const std::string ConsensusModuleManager::getDescription() const {
    return "The consensus module manager keto versions running.";
}

const std::string ConsensusModuleManager::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}

// lifecycle methods
void ConsensusModuleManager::start() {
    modules["ConsensusModule"] = std::make_shared<ConsensusModule>();
    ConsensusServices::init(getConsensusSeedHash(),getConsensusModuleHash());
    EventRegistry::registerEventHandlers();
    KETO_LOG_INFO << "[ConsensusModuleManager] Started the ConsensusModule";

}

void ConsensusModuleManager::postStart() {
    if (consensusServerPtr->require()) {
        consensusServerPtr->start();
    }
    
}

void ConsensusModuleManager::stop() {
    consensusServerPtr.reset();
    EventRegistry::deregisterEventHandlers();
    ConsensusServices::fin();
    modules.clear();
    KETO_LOG_INFO << "[ConsensusModuleManager] The ConsensusModuleManager is being stopped";
}

const std::vector<std::string> ConsensusModuleManager::listModules() {
    std::vector<std::string> keys;
    std::transform(
        this->modules.begin(),
        this->modules.end(),
        std::back_inserter(keys),
        [](const std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>>::value_type 
            &pair){return pair.first;});
    return keys;

}

const std::shared_ptr<keto::module::ModuleInterface> ConsensusModuleManager::getModule(const std::string& name) {
    return modules[name];    
}

boost::shared_ptr<ConsensusModuleManager> ConsensusModuleManager::create_module() {
    return boost::shared_ptr<ConsensusModuleManager>(new ConsensusModuleManager());
}


BOOST_DLL_ALIAS(
    keto::consensus_module::ConsensusModuleManager::create_module,
    create_module                               
)


}
}