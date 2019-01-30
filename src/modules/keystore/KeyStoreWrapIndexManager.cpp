//
// Created by Brett Chaldecott on 2019/01/27.
//

#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"
#include "keto/crypto/KeyBuilder.hpp"

namespace keto {
namespace keystore {

static KeyStoreKeyIndexManagerPtr singleton;

std::string KeyStoreWrapIndexManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

// constructors
KeyStoreWrapIndexManager::KeyStoreWrapIndexManager() {
    this->keyStoreDBPtr = keto::key_store_db::KeyStoreDB::getInstance();
}

KeyStoreWrapIndexManager::~KeyStoreWrapIndexManager() {

}

KeyStoreKeyIndexManagerPtr KeyStoreWrapIndexManager::init() {
    return singleton =  KeyStoreKeyIndexManagerPtr(new KeyStoreWrapIndexManager());
}

void KeyStoreWrapIndexManager::fin() {
    singleton.reset();
}

KeyStoreKeyIndexManagerPtr KeyStoreWrapIndexManager::getInstance() {
    return singleton;
}

// init the store
void KeyStoreWrapIndexManager::initSession() {

}


void KeyStoreWrapIndexManager::clearSession() {
    index.clear();
    derivedKey.reset();
    networkKeys.clear();
}

// set the keys
void KeyStoreWrapIndexManager::setMasterKey(const keto::rpc_protocol::NetworkKeysHelper& keys) {
    std::vector<keto::rpc_protocol::NetworkKeyHelper> keyVector = keys.getNetworkKeys();
    Botan::DataSource_Memory datasource(keyVector[keyVector.size()-1].getKeyBytes_locked());
    std::shared_ptr<Botan::Private_Key> privateKey = Botan::PKCS8::load_key(datasource);

    keto::crypto::KeyBuilder keyBuilder;
    keyBuilder.addPrivateKey(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey())
            .addPrivateKey(privateKey);

    this->derivedKey =
            keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr(
                    new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(keyBuilder.getPrivateKey()));




}

void KeyStoreWrapIndexManager::setWrappingKeys(const keto::rpc_protocol::NetworkKeysHelper& keys) {
    std::vector<keto::rpc_protocol::NetworkKeyHelper> keyVector = keys.getNetworkKeys();

}

int KeyStoreWrapIndexManager::getNumberOfKeys() {

}

keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr KeyStoreWrapIndexManager::getKey(int index) {

}

}
}

