//
// Created by Brett Chaldecott on 2019/01/14.
//

#ifndef KETO_MEMORYVAULTMODULE_HPP
#define KETO_MEMORYVAULTMODULE_HPP

#include <string>
#include <memory>

#include "keto/module/ModuleInterface.hpp"

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace memory_vault_module {

class MemoryVaultModule;
typedef std::shared_ptr<MemoryVaultModule> MemoryVaultModulePtr;

class MemoryVaultModule : public keto::module::ModuleInterface {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    MemoryVaultModule();
    MemoryVaultModule(const MemoryVaultModule & orig) = delete;
    virtual ~MemoryVaultModule();

    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;


};


}
}


#endif //KETO_MEMORYVAULTMODULE_HPP
