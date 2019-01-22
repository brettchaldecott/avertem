//
// Created by Brett Chaldecott on 2019/01/14.
//

#ifndef KETO_EVENTREGISTRY_HPP
#define KETO_EVENTREGISTRY_HPP

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace memory_vault_module {

class EventRegistry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    EventRegistry(const EventRegistry& orig) = delete;
    virtual ~EventRegistry();

    static void registerEventHandlers();
    static void deregisterEventHandlers();

    static keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    static keto::event::Event setModuleSession(const keto::event::Event& event);
    static keto::event::Event setupNodeConsensusSession(const keto::event::Event& event);
    static keto::event::Event consensusSessionAccepted(const keto::event::Event& event);

    static keto::event::Event createVault(const keto::event::Event &event);
    static keto::event::Event addEntry(const keto::event::Event &event);
    static keto::event::Event setEntry(const keto::event::Event &event);
    static keto::event::Event getEntry(const keto::event::Event &event);
    static keto::event::Event removeEntry(const keto::event::Event &event);


private:
    EventRegistry();

};


}
}


#endif //KETO_EVENTREGISTRY_HPP
