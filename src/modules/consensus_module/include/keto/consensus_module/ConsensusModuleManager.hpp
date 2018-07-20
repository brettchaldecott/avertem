/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusModuleManager.hpp
 * Author: ubuntu
 *
 * Created on July 18, 2018, 7:15 AM
 */

#ifndef CONSENSUSMODULEMANAGER_HPP
#define CONSENSUSMODULEMANAGER_HPP

#include <string>
#include <map>
#include <memory>

#include "keto/module/ModuleManagementInterface.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/consensus_module/ConsensusModule.hpp"


namespace keto {
namespace consensus_module {

class ConsensusModuleManager;
typedef std::shared_ptr<ConsensusModuleManager> ConsensusModuleManagerPtr;

class ConsensusModuleManager : public keto::module::ModuleManagementInterface {
public:
    ConsensusModuleManager();
    ConsensusModuleManager(const ConsensusModuleManager& orig) = delete;
    virtual ~ConsensusModuleManager();
    
    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;
    
    // lifecycle methods
    virtual void start();
    virtual void stop();
    
    virtual const std::vector<std::string> listModules();
    virtual const std::shared_ptr<keto::module::ModuleInterface> getModule(const std::string& name);
    
    static boost::shared_ptr<ConsensusModuleManager> create_module();
    
private:
    std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>> modules;
    
    keto::software_consensus::ConsensusHashGeneratorPtr getConsensusSeedHash();
    keto::software_consensus::ConsensusHashGeneratorPtr getConsensusModuleHash();
};

}
}

#endif /* CONSENSUSMODULEMANAGER_HPP */

