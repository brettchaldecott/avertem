/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.hpp
 * Author: ubuntu
 *
 * Created on February 17, 2018, 12:29 PM
 */

#ifndef EVENTREGISTRY_HPP
#define EVENTREGISTRY_HPP

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/key_store_utils/Events.hpp"

namespace keto {
namespace keystore {


class EventRegistry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    EventRegistry(const EventRegistry& orig) = delete;
    virtual ~EventRegistry();

    static keto::event::Event requestPassword(const keto::event::Event& event);

    static keto::event::Event requestSessionKey(const keto::event::Event& event);
    static keto::event::Event removeSessionKey(const keto::event::Event& event);
    
    static keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    static keto::event::Event setModuleSession(const keto::event::Event& event);
    static keto::event::Event consensusSessionAccepted(const keto::event::Event& event);
    static keto::event::Event consensusProtocolCheck(const keto::event::Event& event);
    static keto::event::Event consensusHeartbeat(const keto::event::Event& event);
    
    static keto::event::Event reencryptTransaction(const keto::event::Event& event);
    static keto::event::Event encryptTransaction(const keto::event::Event& event);
    static keto::event::Event decryptTransaction(const keto::event::Event& event);

    static keto::event::Event encryptAsn1(const keto::event::Event& event);
    static keto::event::Event decryptAsn1(const keto::event::Event& event);

    static keto::event::Event encryptNetworkBytes(const keto::event::Event& event);
    static keto::event::Event decryptNetworkBytes(const keto::event::Event& event);

    static keto::event::Event getNetworkSessionKeys(const keto::event::Event& event);
    static keto::event::Event setNetworkSessionKeys(const keto::event::Event& event);

    static  keto::event::Event getMasterKeys(const keto::event::Event& event);
    static  keto::event::Event setMasterKeys(const keto::event::Event& event);

    static keto::event::Event getNetworkKeys(const keto::event::Event& event);
    static keto::event::Event setNetworkKeys(const keto::event::Event& event);

    static keto::event::Event isMaster(const keto::event::Event& event);

    static void registerEventHandlers();
    static void deregisterEventHandlers();
private:
    EventRegistry();
    
};


}
}

#endif /* EVENTREGISTRY_HPP */

