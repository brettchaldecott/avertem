//
// Created by Brett Chaldecott on 2018/11/27.
//

#include "KeyStore.pb.h"

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/keystore/Exception.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/crypto/KeyBuilder.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/keystore/MasterKeyManager.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


#include "keto/keystore/KeyStoreWrapIndexManager.hpp"


namespace keto {
namespace keystore {

static KeyStoreStorageManagerPtr singleton;

std::string KeyStoreStorageManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreStorageManager::KeyStoreStorageManager() {
    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();

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


// init the store
void KeyStoreStorageManager::initSession() {

}

void KeyStoreStorageManager::clearSession() {

}


// get the key loader
std::shared_ptr<keto::crypto::KeyLoader> KeyStoreStorageManager::getKeyLoader() {
    return this->keyLoaderPtr;
}

keto::event::Event KeyStoreStorageManager::requestPassword(
        const keto::event::Event& event) {
    keto::proto::PasswordRequest request = keto::server_common::fromEvent<keto::proto::PasswordRequest>(event);

    keto::proto::PasswordResponse passwordResponse;
    passwordResponse.set_identifier(request.identifier());
    keto::crypto::SecureVector password = getPassword(request.identifier());
    passwordResponse.set_password(keto::crypto::SecureVectorUtils().copySecureToString(password));
    return keto::server_common::toEvent<keto::proto::PasswordResponse>(passwordResponse);
}

keto::crypto::SecureVector KeyStoreStorageManager::getPassword(const std::string& entryId) {
    KeyStoreWrapIndexDecryptorPtr decryptorPtr = KeyStoreWrapIndexManager::getInstance()->getDecryptor();

    keto::crypto::SecureVector wrapperValue;
    if (this->keyStoreDBPtr->getValue(entryId,*decryptorPtr,wrapperValue)) {
        return wrapperValue;
    }
    keto::crypto::PasswordPipeLine passwordPipeLine;
    wrapperValue = passwordPipeLine.generatePassword(keto::crypto::SecureVectorUtils().copyStringToSecure(entryId));

    KeyStoreWrapIndexEncryptorPtr encryptorPtr = KeyStoreWrapIndexManager::getInstance()->getEncryptor();
    this->keyStoreDBPtr->setValue(entryId,wrapperValue,*encryptorPtr);
    return wrapperValue;
}

}
}


