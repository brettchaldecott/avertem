/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   KeystoreModuleManager.cpp
 * Author: Brett Chaldecott
 * 
 * Created on January 20, 2018, 4:40 PM
 */

#include <boost/dll/alias.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/shared_ptr.hpp>

#include "keto/common/Log.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/keystore/KeystoreModuleManager.hpp"
#include "keto/keystore/KeystoreModuleManagerMisc.hpp"
#include "keto/keystore/KeyStoreService.hpp"
#include "keto/keystore/EventRegistry.hpp"
#include "keto/keystore/EventRegistry.hpp"
#include "keto/keystore/ConsensusService.hpp"
#include "keto/keystore/TransactionEncryptionService.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/MasterKeyManager.hpp"
#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"

#include "keto/memory_vault_session/MemoryVaultSession.hpp"

namespace keto {
namespace keystore {

std::string KeystoreModuleManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
KeystoreModuleManager::KeystoreModuleManager() {
}


KeystoreModuleManager::~KeystoreModuleManager() {
}

// meta methods
const std::string KeystoreModuleManager::getName() const {
    return "KeystoreModuleManager";
}

const std::string KeystoreModuleManager::getDescription() const {
    return "The keystore module manager responsible for managing the keystore process.";
}

const std::string KeystoreModuleManager::getVersion() const {
    return keto::common::MetaInfo::VERSION;
}

// lifecycle methods
void KeystoreModuleManager::start() {
    KeyStoreService::init();
    modules["KeystoreModule"] = std::make_shared<KeystoreModule>();
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGeneratorPtr =
                                                                this->getConsensusHash();
    keto::memory_vault_session::MemoryVaultSession::init(consensusHashGeneratorPtr,Constants::MODULE_NAME);
    NetworkSessionKeyManager::init(consensusHashGeneratorPtr);
    ConsensusService::init(consensusHashGeneratorPtr);
    KeyStoreStorageManager::init();
    KeyStoreWrapIndexManager::init();
    MasterKeyManager::init();
    TransactionEncryptionService::init();
    EventRegistry::registerEventHandlers();
    KETO_LOG_INFO << "[KeystoreModuleManager] Started the KeystoreModuleManager";
}

void KeystoreModuleManager::stop() {
    EventRegistry::deregisterEventHandlers();
    TransactionEncryptionService::fin();
    MasterKeyManager::fin();
    KeyStoreWrapIndexManager::fin();
    KeyStoreStorageManager::fin();
    KeyStoreStorageManager::fin();
    KeyStoreService::fin();
    NetworkSessionKeyManager::fin();
    keto::memory_vault_session::MemoryVaultSession::fin();
    ConsensusService::fin();
    modules.clear();
    KETO_LOG_INFO << "[KeystoreModuleManager] The KeystoreModuleManager is being stopped";
}

void KeystoreModuleManager::postStart() {
    //keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
    //KeyStoreStorageManager::getInstance()->initStore();
    //transactionPtr->commit();
    KETO_LOG_INFO << "[KeystoreModuleManager] The keystore post start";
}


const std::vector<std::string> KeystoreModuleManager::listModules() {
    std::vector<std::string> keys;
    std::transform(
        this->modules.begin(),
        this->modules.end(),
        std::back_inserter(keys),
        [](const std::map<std::string,std::shared_ptr<keto::module::ModuleInterface>>::value_type 
            &pair){return pair.first;});
    return keys;
}

const std::shared_ptr<keto::module::ModuleInterface> KeystoreModuleManager::getModule(const std::string& name) {
    return modules[name];
}

boost::shared_ptr<KeystoreModuleManager> KeystoreModuleManager::create_module() {
    return boost::shared_ptr<KeystoreModuleManager>(new KeystoreModuleManager());
}

BOOST_DLL_ALIAS(
    keto::keystore::KeystoreModuleManager::create_module,
    create_module                               
)


}
}
