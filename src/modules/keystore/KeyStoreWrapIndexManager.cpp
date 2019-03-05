//
// Created by Brett Chaldecott on 2019/01/27.
//

#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"
#include "keto/crypto/KeyBuilder.hpp"
#include "keto/keystore/Exception.hpp"

namespace keto {
namespace keystore {

static KeyStoreWrapIndexManagerPtr singleton;

std::string KeyStoreWrapIndexManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

// constructors
KeyStoreWrapIndexManager::KeyStoreWrapIndexManager() {
    this->keyStoreDBPtr = keto::key_store_db::KeyStoreDB::getInstance();
}

KeyStoreWrapIndexManager::~KeyStoreWrapIndexManager() {

}

KeyStoreWrapIndexManagerPtr KeyStoreWrapIndexManager::init() {
    return singleton =  KeyStoreWrapIndexManagerPtr(new KeyStoreWrapIndexManager());
}

void KeyStoreWrapIndexManager::fin() {
    singleton.reset();
}

KeyStoreWrapIndexManagerPtr KeyStoreWrapIndexManager::getInstance() {
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

    loadWrapperIndex();


}

void KeyStoreWrapIndexManager::setWrappingKeys(const keto::rpc_protocol::NetworkKeysHelper& keys) {
    std::vector<keto::rpc_protocol::NetworkKeyHelper> keyVector = keys.getNetworkKeys();

    for (keto::rpc_protocol::NetworkKeyHelper key : keys.getNetworkKeys()) {
        if (!this->networkKeys.count(key.getHash())) {
            KeyStoreWrapEntryPtr keyStoreWrapEntryPtr(new KeyStoreWrapEntry(key));
            this->networkKeys[keyStoreWrapEntryPtr->getHash()] = keyStoreWrapEntryPtr;
            this->index.push_back(keyStoreWrapEntryPtr->getHash());
        } else {
            KeyStoreWrapEntryPtr keyStoreWrapEntryPtr = this->networkKeys[key.getHash()];
            keyStoreWrapEntryPtr->setKey(key);
        }
    }

    setWrapperIndex();
}

// methods to get the encryptor and decryptor
KeyStoreWrapIndexEncryptorPtr KeyStoreWrapIndexManager::getEncryptor() {
        return KeyStoreWrapIndexEncryptorPtr(new KeyStoreWrapIndexEncryptor(this));
}

KeyStoreWrapIndexDecryptorPtr KeyStoreWrapIndexManager::getDecryptor() {
    return KeyStoreWrapIndexDecryptorPtr(new KeyStoreWrapIndexDecryptor(this));
}

int KeyStoreWrapIndexManager::getNumberOfKeys() {
    return this->index.size();
}

keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr KeyStoreWrapIndexManager::getKey(int index) {
    KeyStoreWrapEntryPtr keyStoreWrapEntryPtr = this->networkKeys[this->index[index]];
    if (!keyStoreWrapEntryPtr) {
        BOOST_THROW_EXCEPTION(keto::keystore::InvalidKeyIndexConfigured(
                                      "The session key identifier is invalid."));
    }
    return keyStoreWrapEntryPtr->getDerivedKey();
}



void KeyStoreWrapIndexManager::loadWrapperIndex() {
    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->derivedKey->getPrivateKey());


    std::string wrapperValue;
    if (this->keyStoreDBPtr->getValue(Constants::KEY_STORE_DB::KEY_STORE_WRAPPER_INDEX,onionKeys,wrapperValue)) {
        nlohmann::json json = nlohmann::json::parse(wrapperValue);
        for (nlohmann::json::iterator it = json.begin(); it != json.end(); ++it) {
            KeyStoreWrapEntryPtr keyStoreWrapEntryPtr(new KeyStoreWrapEntry(*it));
            this->networkKeys[keyStoreWrapEntryPtr->getHash()] = keyStoreWrapEntryPtr;
            this->index.push_back(keyStoreWrapEntryPtr->getHash());
        }
    }
}

void KeyStoreWrapIndexManager::setWrapperIndex() {
    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->derivedKey->getPrivateKey());

    nlohmann::json json;
    for (std::vector<uint8_t> index : this->index) {
        KeyStoreWrapEntryPtr keyStoreWrapEntryPtr = this->networkKeys[index];
        json.push_back(keyStoreWrapEntryPtr->getJson());
    }
    std::string wrapperValue = json.dump();
    this->keyStoreDBPtr->setValue(Constants::KEY_STORE_DB::KEY_STORE_WRAPPER_INDEX,wrapperValue,onionKeys);
}

}
}

