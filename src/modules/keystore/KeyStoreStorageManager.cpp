//
// Created by Brett Chaldecott on 2018/11/27.
//

#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/keystore/Exception.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/crypto/KeyBuilder.hpp"



namespace keto {
namespace keystore {

static KeyStoreStorageManagerPtr singleton;

std::string KeyStoreStorageManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreStorageManager::KeyStoreStorageManager() : master(false) {
    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::IS_MASTER)) {
        if (config->getVariablesMap()[Constants::IS_MASTER].as<std::string>().compare(Constants::IS_MASTER_TRUE)) {
            this->master = true;
        }
    }

    // load the key information
    if (!config->getVariablesMap().count(Constants::PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::keystore::PrivateKeyNotConfiguredException());
    }
    std::string privateKeyPath =
            config->getVariablesMap()[Constants::PRIVATE_KEY].as<std::string>();

    if (!config->getVariablesMap().count(Constants::PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::keystore::PublicKeyNotConfiguredException());
    }
    std::string publicKeyPath =
            config->getVariablesMap()[Constants::PUBLIC_KEY].as<std::string>();

    keyLoaderPtr = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
                                                             publicKeyPath);

    keto::key_store_db::KeyStoreDB::init(this->keyLoaderPtr->getPrivateKey());
    this->keyStoreDBPtr = keto::key_store_db::KeyStoreDB::getInstance();



}


KeyStoreStorageManager::~KeyStoreStorageManager() {

}


KeyStoreStorageManagerPtr KeyStoreStorageManager::init() {
    return singleton = KeyStoreStorageManagerPtr(new KeyStoreStorageManager());
}

void KeyStoreStorageManager::fin() {
    singleton.reset();
    keto::key_store_db::KeyStoreDB::fin();
}

KeyStoreStorageManagerPtr KeyStoreStorageManager::getInstance() {
    return singleton;
}


void KeyStoreStorageManager::initStore() {
    //std::cout << "Init the store" << std::endl;
    if (this->isMaster()) {
        keto::crypto::KeyBuilder keyBuilder;
        keyBuilder.addPrivateKey(this->keyLoaderPtr->getPrivateKey())
                .addPrivateKey(this->keyLoaderPtr->getPrivateKey()).addPublicKey(this->keyLoaderPtr->getPublicKey());
        //std::cout << "Setup the  derived key" << std::endl;
        this->setDerivedKey(keyBuilder.getPrivateKey());
        unlockStore();
    }
}


void KeyStoreStorageManager::unlockStore() {

    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(this->keyLoaderPtr->getPrivateKey());
    //std::cout << "Unlock the store with the derived key" << std::endl;
    onionKeys.push_back(this->derivedKey->getPrivateKey());
    std::string value;
    if (!this->keyStoreDBPtr->getValue(Constants::KEY_STORE_DB::KEY_STORE_MASTER_ENTRY,onionKeys,value)) {
        this->masterKeyStoreEntry = KeyStoreEntryPtr(new KeyStoreEntry());
        this->keyStoreDBPtr->setValue(Constants::KEY_STORE_DB::KEY_STORE_MASTER_ENTRY,this->masterKeyStoreEntry->getJson(),onionKeys);
    } else {
        this->masterKeyStoreEntry = KeyStoreEntryPtr(new KeyStoreEntry(value));
    }
}


void KeyStoreStorageManager::setDerivedKey(std::shared_ptr<Botan::Private_Key> derivedKey) {
    this->derivedKey =
            keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr(
                    new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(derivedKey));
}


bool KeyStoreStorageManager::isMaster() const {
    return master;
}


keto::event::Event KeyStoreStorageManager::getNetworkKeys(const keto::event::Event& event) {
    
    return event;
}

keto::event::Event KeyStoreStorageManager::setNetworkKeys(const keto::event::Event& event) {

    return event;
}

}
}


