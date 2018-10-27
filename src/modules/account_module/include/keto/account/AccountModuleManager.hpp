/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountModuleManager.hpp
 * Author: ubuntu
 *
 * Created on March 3, 2018, 12:17 PM
 */

#ifndef ACCOUNTMODULEMANAGER_HPP
#define ACCOUNTMODULEMANAGER_HPP

#include <string>
#include <map>
#include <memory>

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/module/ModuleManagementInterface.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace account {

class AccountModuleManager : public keto::module::ModuleManagementInterface {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    AccountModuleManager();
    AccountModuleManager(const AccountModuleManager& orig) = delete;
    virtual ~AccountModuleManager();
    
    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;
    
    // lifecycle methods
    virtual void start();
    virtual void stop();
    
    virtual const std::vector<std::string> listModules();
    virtual const std::shared_ptr<keto::module::ModuleInterface> getModule(const std::string& name);
    
    static boost::shared_ptr<AccountModuleManager> create_module();

private:
    std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>> modules;
    
    keto::software_consensus::ConsensusHashGeneratorPtr getConsensusHash();
};


}
}


#endif /* ACCOUNTMODULEMANAGER_HPP */

