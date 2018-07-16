/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventServiceInterface.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 18, 2018, 12:07 PM
 */

#ifndef KETO_EVENTSERVICEINTERFACE_HPP
#define KETO_EVENTSERVICEINTERFACE_HPP

#include <string>
#include <memory>

#include "keto/module/ModuleManager.hpp"
#include "keto/module/ModuleInterface.hpp"
#include "keto/event/Event.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace event {

typedef Event (*handler)(const Event&);
    
class EventServiceInterface {
public:
    inline static std::string getVersion() {
        return OBFUSCATED("$Id: $");
    };
    
    static constexpr const char* KETO_EVENT_SERVICE_MODULE = "event_service";
    
    virtual void triggerEvent(const Event& event) = 0;
    virtual Event processEvent(const Event& event) = 0;
    virtual void registerEventHandler(const std::string& name, handler handlerMethod) = 0;
    virtual void deregisterEventHandler(const std::string& name) = 0;
};

}
}


#endif /* EVENTSERVICEINTERFACE_HPP */

