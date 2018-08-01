/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SandboxModuleManager.hpp
 * Author: ubuntu
 *
 * Created on January 20, 2018, 4:25 PM
 */

#ifndef SANDBOXMODULEMANAGER_HPP
#define SANDBOXMODULEMANAGER_HPP

#include <string>
#include <map>
#include <memory>

#include "keto/module/ModuleManagementInterface.hpp"

#include "keto/sandbox/SandboxModule.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace sandbox {

class SandboxModuleManager : public keto::module::ModuleManagementInterface {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    SandboxModuleManager();
    SandboxModuleManager(const SandboxModuleManager& orig) = delete;
    virtual ~SandboxModuleManager();
    
    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;
    
    // lifecycle methods
    virtual void start();
    virtual void stop();
    virtual void postStart();
    
    virtual const std::vector<std::string> listModules();
    virtual const std::shared_ptr<keto::module::ModuleInterface> getModule(const std::string& name);
    
    static boost::shared_ptr<SandboxModuleManager> create_module();

private:
    std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>> modules;
    
    keto::software_consensus::ConsensusHashGeneratorPtr getConsensusHash();
};

}
}

#endif /* SANDBOXMODULEMANAGER_HPP */

