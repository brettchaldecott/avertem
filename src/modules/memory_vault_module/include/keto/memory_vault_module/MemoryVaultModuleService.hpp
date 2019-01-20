//
// Created by Brett Chaldecott on 2019/01/16.
//

#ifndef KETO_MEMORYVAULTMODULESERVICE_HPP
#define KETO_MEMORYVAULTMODULESERVICE_HPP

#include <string>
#include <memory>

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace memory_vault_module {

class MemoryVaultModuleService;
typedef std::shared_ptr<MemoryVaultModuleService> MemoryVaultModuleServicePtr;

class MemoryVaultModuleService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    MemoryVaultModuleService();
    MemoryVaultModuleService(const MemoryVaultModuleService& orig) = delete;
    ~MemoryVaultModuleService();

    static MemoryVaultModuleServicePtr init();
    static MemoryVaultModuleServicePtr getInstance();
    static void fin();

    keto::event::Event createVault(const keto::event::Event &event);
    keto::event::Event addEntry(const keto::event::Event &event);
    keto::event::Event setEntry(const keto::event::Event &event);
    keto::event::Event getEntry(const keto::event::Event &event);
    keto::event::Event removeEntry(const keto::event::Event &event);

private:
};


}
}


#endif //KETO_MEMORYVAULTMODULESERVICE_HPP
