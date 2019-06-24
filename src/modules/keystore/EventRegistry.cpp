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
#include <sstream>

#include <keto/keystore/NetworkSessionKeyManager.hpp>

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
#include "keto/keystore/MasterKeyManager.hpp"
#include "keto/keystore/EncryptionService.hpp"
#include "keto/keystore/Exception.hpp"
#include "keto/software_consensus/ConsensusStateManager.hpp"


namespace keto {
namespace keystore {


std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}

keto::event::Event EventRegistry::requestPassword(
        const keto::event::Event& event) {
    return KeyStoreStorageManager::getInstance()->requestPassword(event);
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

keto::event::Event EventRegistry::consensusProtocolCheck(const keto::event::Event& event) {

    return event;
}

keto::event::Event EventRegistry::consensusHeartbeat(const keto::event::Event& event) {

    return event;
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

keto::event::Event EventRegistry::encryptAsn1(const keto::event::Event& event) {
    return EncryptionService::getInstance()->encryptAsn1(event);
}

keto::event::Event EventRegistry::decryptAsn1(const keto::event::Event& event) {
    return EncryptionService::getInstance()->decryptAsn1(event);
}

keto::event::Event EventRegistry::encryptNetworkBytes(const keto::event::Event& event) {
    return EncryptionService::getInstance()->encryptNetworkBytes(event);
}

keto::event::Event EventRegistry::decryptNetworkBytes(const keto::event::Event& event) {
    return EncryptionService::getInstance()->decryptNetworkBytes(event);
}


keto::event::Event EventRegistry::getNetworkSessionKeys(const keto::event::Event& event) {
    if (keto::software_consensus::ConsensusStateManager::getInstance()->getState() !=
        keto::software_consensus::ConsensusStateManager::ACCEPTED) {
        std::stringstream ss;
        ss << "[EventRegistry::getNetworkSessionKeys]Session state is currently [" <<
            keto::software_consensus::ConsensusStateManager::getInstance()->getState() << "]";
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException(ss.str()));
    }

    return NetworkSessionKeyManager::getInstance()->getNetworkSessionKeys(event);
}

keto::event::Event EventRegistry::setNetworkSessionKeys(const keto::event::Event& event) {
    return NetworkSessionKeyManager::getInstance()->setNetworkSessionKeys(event);
}

keto::event::Event EventRegistry::getMasterKeys(const keto::event::Event& event) {
    if (keto::software_consensus::ConsensusStateManager::getInstance()->getState() !=
        keto::software_consensus::ConsensusStateManager::ACCEPTED) {
        std::stringstream ss;
        ss << "[EventRegistry::getMasterKeys]Session state is currently [" <<
           keto::software_consensus::ConsensusStateManager::getInstance()->getState() << "]";
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException(ss.str()));
    }
    return MasterKeyManager::getInstance()->getMasterKey(event);
}

keto::event::Event EventRegistry::setMasterKeys(const keto::event::Event& event) {
    return MasterKeyManager::getInstance()->setMasterKey(event);
}

keto::event::Event EventRegistry::getNetworkKeys(const keto::event::Event& event) {
    if (keto::software_consensus::ConsensusStateManager::getInstance()->getState() !=
        keto::software_consensus::ConsensusStateManager::ACCEPTED) {
        std::stringstream ss;
        ss << "[EventRegistry::getNetworkKeys]Session state is currently [" <<
           keto::software_consensus::ConsensusStateManager::getInstance()->getState() << "]";
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    }
    return MasterKeyManager::getInstance()->getWrappingKeys(event);
}

keto::event::Event EventRegistry::setNetworkKeys(const keto::event::Event& event) {
    return MasterKeyManager::getInstance()->setWrappingKeys(event);
}

keto::event::Event EventRegistry::isMaster(const keto::event::Event& event) {
    return MasterKeyManager::getInstance()->isMaster(event);
}

void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::REQUEST_PASSWORD,
            &keto::keystore::EventRegistry::requestPassword);

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
            keto::server_common::Events::ENCRYPT_ASN1::ENCRYPT,
            &EventRegistry::encryptAsn1);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::ENCRYPT_ASN1::DECRYPT,
            &EventRegistry::decryptAsn1);

    keto::server_common::registerEventHandler(
            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::ENCRYPT,
            &EventRegistry::encryptNetworkBytes);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::DECRYPT,
            &EventRegistry::decryptNetworkBytes);
    
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
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::KEYSTORE,
            &EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::CONSENSUS_HEARTBEAT::KEYSTORE,
            &EventRegistry::consensusHeartbeat);

    keto::server_common::registerEventHandler(
            keto::server_common::Events::GET_NETWORK_SESSION_KEYS,
            &EventRegistry::getNetworkSessionKeys);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::SET_NETWORK_SESSION_KEYS,
            &EventRegistry::setNetworkSessionKeys);

    keto::server_common::registerEventHandler(
            keto::server_common::Events::GET_MASTER_NETWORK_KEYS,
            &EventRegistry::getMasterKeys);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::SET_MASTER_NETWORK_KEYS,
            &EventRegistry::setMasterKeys);

    keto::server_common::registerEventHandler(
            keto::server_common::Events::GET_NETWORK_KEYS,
            &EventRegistry::getNetworkKeys);
    keto::server_common::registerEventHandler(
            keto::server_common::Events::SET_NETWORK_KEYS,
            &EventRegistry::setNetworkKeys);

    keto::server_common::registerEventHandler(
            keto::server_common::Events::IS_MASTER,
            &EventRegistry::isMaster);

}


void EventRegistry::deregisterEventHandlers() {

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::IS_MASTER);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::GET_NETWORK_KEYS);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SET_NETWORK_KEYS);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::GET_MASTER_NETWORK_KEYS);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SET_MASTER_NETWORK_KEYS);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::GET_NETWORK_SESSION_KEYS);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SET_NETWORK_SESSION_KEYS);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::KEYSTORE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CONSENSUS_HEARTBEAT::KEYSTORE);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::ENCRYPT);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::DECRYPT);

    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::ENCRYPT_ASN1::DECRYPT);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::ENCRYPT_ASN1::ENCRYPT);

    keto::server_common::deregisterEventHandler(
            keto::key_store_utils::Events::TRANSACTION::REENCRYPT_TRANSACTION);
    keto::server_common::deregisterEventHandler(
            keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION);
    keto::server_common::deregisterEventHandler(
            keto::key_store_utils::Events::TRANSACTION::DECRYPT_TRANSACTION);

    keto::server_common::deregisterEventHandler(keto::server_common::Events::REMOVE_SESSION_KEY);
    keto::server_common::deregisterEventHandler(keto::server_common::Events::REQUEST_SESSION_KEY);

    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::REQUEST_PASSWORD);
}


}
}
