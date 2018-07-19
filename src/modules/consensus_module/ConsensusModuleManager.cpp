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

namespace keto {
namespace consensus_module {


ConsensusModuleManager::ConsensusModuleManager() {
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
    KETO_LOG_INFO << "[ConsensusModuleManager] Started the ConsensusModule";

}

void ConsensusModuleManager::stop() {
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