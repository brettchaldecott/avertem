/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.hpp
 * Author: ubuntu
 *
 * Created on March 6, 2018, 1:42 PM
 */

#ifndef EVENTREGISTRY_HPP
#define EVENTREGISTRY_HPP

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace account {

class EventRegistry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    EventRegistry(const EventRegistry& orig) = delete;
    virtual ~EventRegistry();
    
    static keto::event::Event checkAccount(const keto::event::Event& event);
    static keto::event::Event getNodeAccountRouting(const keto::event::Event& event);
    static keto::event::Event applyDirtyTransaction(const keto::event::Event& event);
    static keto::event::Event applyTransaction(const keto::event::Event& event);
    static keto::event::Event sparqlQuery(const keto::event::Event& event);
    static keto::event::Event dirtySparqlQueryWithResultSet(const keto::event::Event& event);
    static keto::event::Event sparqlQueryWithResultSet(const keto::event::Event& event);
    static keto::event::Event getContract(const keto::event::Event& event);
    static keto::event::Event clearDirty(const keto::event::Event& event);
    
    static keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    static keto::event::Event setModuleSession(const keto::event::Event& event);
    static keto::event::Event consensusSessionAccepted(const keto::event::Event& event);
    static keto::event::Event consensusProtocolCheck(const keto::event::Event& event);
    static keto::event::Event consensusHeartbeat(const keto::event::Event& event);
    
    static void registerEventHandlers();
    static void deregisterEventHandlers();
private:
    EventRegistry();
    
    
};


}
}


#endif /* EVENTREGISTRY_HPP */

