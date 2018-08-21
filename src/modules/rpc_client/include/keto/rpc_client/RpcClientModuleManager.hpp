/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcClientModuleManager.hpp
 * Author: ubuntu
 *
 * Created on January 20, 2018, 1:40 PM
 */

#ifndef RPCCLIENTMODULEMANAGER_HPP
#define RPCCLIENTMODULEMANAGER_HPP

#include <string>
#include <map>
#include <memory>

#include "keto/module/ModuleManagementInterface.hpp"
#include "keto/rpc_client/RpcClientModule.hpp"
#include "keto/rpc_client/RpcSessionManager.hpp"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace rpc_client {

class RpcClientModuleManager : public keto::module::ModuleManagementInterface {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    RpcClientModuleManager();
    RpcClientModuleManager(const RpcClientModuleManager& orig) = delete;
    virtual ~RpcClientModuleManager();
    
    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;
    
    // lifecycle methods
    virtual void start();
    virtual void postStart();
    virtual void stop();
    
    virtual const std::vector<std::string> listModules();
    virtual const std::shared_ptr<keto::module::ModuleInterface> getModule(const std::string& name);
    
    static boost::shared_ptr<RpcClientModuleManager> create_module();

    
private:
    std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>> modules;
    
    keto::software_consensus::ConsensusHashGeneratorPtr getConsensusHash();

};

}
}

#endif /* RPCCLIENTMODULEMANAGER_HPP */

