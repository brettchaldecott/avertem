/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.cpp
 * Author: ubuntu
 * 
 * Created on February 17, 2018, 12:29 PM
 */

#include <iostream>

#include "keto/keystore/EventRegistry.hpp"
#include "keto/event/EventServiceInterface.hpp"
#include "keto/keystore/KeyStoreService.hpp"
#include "keto/keystore/SessionKeyManager.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/keystore/EventRegistry.hpp"
#include "keto/keystore/ConsensusService.hpp"

namespace keto {
namespace keystore {


EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}

keto::event::Event EventRegistry::requestSessionKey(
    const keto::event::Event& event) {
    return KeyStoreService::getInstance()->getSessionKeyManager()->requestKey(event);
}


keto::event::Event EventRegistry::removeSessionKey(
    const keto::event::Event& event) {
    return KeyStoreService::getInstance()->getSessionKeyManager()->removeKey(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}



void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::REQUEST_SESSION_KEY,
            &keto::keystore::EventRegistry::requestSessionKey);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::REMOVE_SESSION_KEY,
            &EventRegistry::removeSessionKey);
    
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS::KEYSTORE,
            &EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE,
            &EventRegistry::setModuleSession);
}


void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::REMOVE_SESSION_KEY);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::REQUEST_SESSION_KEY);
}


}
}