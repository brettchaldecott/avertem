//
// Created by Brett Chaldecott on 2019/01/14.
//

#ifndef KETO_MEMORYVAULTMODULEMANAGER_HPP
#define KETO_MEMORYVAULTMODULEMANAGER_HPP



#include "keto/module/ModuleManagementInterface.hpp"
#include "keto/module/ModuleInterface.hpp"

#include "keto/memory_vault_module/MemoryVaultModule.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace memory_vault_module {

class MemoryVaultModuleManager : public keto::module::ModuleManagementInterface {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    MemoryVaultModuleManager();
    MemoryVaultModuleManager(const MemoryVaultModuleManager& orig) = delete;
    virtual ~MemoryVaultModuleManager();

    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;

    // lifecycle methods
    virtual void start();
    virtual void stop();

    virtual const std::vector<std::string> listModules();
    virtual const std::shared_ptr<keto::module::ModuleInterface> getModule(const std::string& name);

    static boost::shared_ptr<MemoryVaultModuleManager> create_module();

private:
    std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>> modules;

    keto::software_consensus::ConsensusHashGeneratorPtr getConsensusHash();

};


}
}

#endif //KETO_MEMORYVAULTMODULEMANAGER_HPP
