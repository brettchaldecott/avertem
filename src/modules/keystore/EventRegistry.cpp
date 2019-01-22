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
#include "keto/keystore/TransactionEncryptionService.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"


namespace keto {
namespace keystore {


std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

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

keto::event::Event EventRegistry::consensusSessionAccepted(const keto::event::Event& event) {
    return ConsensusService::getInstance()->consensusSessionAccepted(event);
}

keto::event::Event EventRegistry::reencryptTransaction(const keto::event::Event& event) {
    return TransactionEncryptionService::getInstance()->reencryptTransaction(event);
}

keto::event::Event EventRegistry::encryptTransaction(const keto::event::Event& event) {
    return TransactionEncryptionService::getInstance()->encryptTransaction(event);
}

keto::event::Event EventRegistry::decryptTransaction(const keto::event::Event& event) {
    return TransactionEncryptionService::getInstance()->decryptTransaction(event);
}

keto::event::Event EventRegistry::getNetworkKeys(const keto::event::Event& event) {
    return KeyStoreStorageManager::getInstance()->getNetworkKeys(event);
}

keto::event::Event EventRegistry::setNetworkKeys(const keto::event::Event& event) {
    return KeyStoreStorageManager::getInstance()->setNetworkKeys(event);
}

void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::REQUEST_SESSION_KEY,
            &keto::keystore::EventRegistry::requestSessionKey);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::REMOVE_SESSION_KEY,
            &EventRegistry::removeSessionKey);
    
    keto::server_common::registerEventHandler(
            keto::key_store_utils::Events::TRANSACTION::REENCRYPT_TRANSACTION,
            &EventRegistry::reencryptTransaction);
    keto::server_common::registerEventHandler(
            keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION,
            &EventRegistry::encryptTransaction);
    keto::server_common::registerEventHandler(
            keto::key_store_utils::Events::TRANSACTION::DECRYPT_TRANSACTION,
            &EventRegistry::decryptTransaction);
    
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS::KEYSTORE,
            &EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE,
            &EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::KEYSTORE,
            &EventRegistry::consensusSessionAccepted);

    keto::server_common::registerEventHandler(
            keto::server_common::Events::GET_NETWORK_KEYS,
            &EventRegistry::getNetworkKeys);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::SET_NETWORK_KEYS,
            &EventRegistry::setNetworkKeys);

}


void EventRegistry::deregisterEventHandlers() {

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::GET_NETWORK_KEYS);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SET_NETWORK_KEYS);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::KEYSTORE);
    
    keto::server_common::deregisterEventHandler(
            keto::key_store_utils::Events::TRANSACTION::REENCRYPT_TRANSACTION);
    keto::server_common::deregisterEventHandler(
            keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION);
    keto::server_common::deregisterEventHandler(
            keto::key_store_utils::Events::TRANSACTION::DECRYPT_TRANSACTION);
    
    keto::server_common::deregisterEventHandler(keto::server_common::Events::REMOVE_SESSION_KEY);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::REQUEST_SESSION_KEY);
}


}
}
