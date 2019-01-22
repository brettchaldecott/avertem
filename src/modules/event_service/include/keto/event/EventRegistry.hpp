/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.hpp
 * Author: ubuntu
 *
 * Created on March 8, 2018, 3:15 AM
 */

#ifndef EVENTREGISTRY_HPP
#define EVENTREGISTRY_HPP

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace event {

class EventRegistry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    EventRegistry(const EventRegistry& orig) = delete;
    virtual ~EventRegistry();
    
    static void registerEventHandlers();
    
    static keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    static keto::event::Event setModuleSession(const keto::event::Event& event);
    static keto::event::Event consensusSessionAccepted(const keto::event::Event& event);

private:
    EventRegistry();
    
};

}
}

#endif /* EVENTREGISTRY_HPP */

