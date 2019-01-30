//
// Created by Brett Chaldecott on 2019/01/27.
//

#include "keto/crypto/KeyBuilder.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/keystore/KeyStoreWrapEntry.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/rpc_protocol/NetworkKeyHelper.hpp"


namespace keto {
namespace keystore {


std::string KeyStoreWrapEntry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreWrapEntry::KeyStoreWrapEntry(const keto::proto::NetworkKey& networkKey) {
    keto::rpc_protocol::NetworkKeyHelper networkKeyHelper(networkKey);

    this->hash = networkKeyHelper.getHash();
    this->active = networkKeyHelper.getActive();

    Botan::DataSource_Memory datasource(networkKeyHelper.getKeyBytes_locked());
    std::shared_ptr<Botan::Private_Key> privateKey = Botan::PKCS8::load_key(datasource);

    keto::crypto::KeyBuilder keyBuilder;
    keyBuilder.addPrivateKey(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey())
            .addPrivateKey(privateKey);

    this->derivedKey =
            keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr(
                    new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(keyBuilder.getPrivateKey()));
}

KeyStoreWrapEntry::KeyStoreWrapEntry(const nlohmann::json& jsonEntry) {
    this->hash = Botan::hex_decode(jsonEntry[Constants::NETWORK_KEY_HASH],true);
    this->active = jsonEntry[Constants::NETWORK_KEY_ACTIVE];
}

KeyStoreWrapEntry::~KeyStoreWrapEntry() {

}

std::vector<uint8_t> KeyStoreWrapEntry::getHash() {
    return this->hash;
}

bool KeyStoreWrapEntry::getActive() {
    return this->active;
}

KeyStoreWrapEntry& KeyStoreWrapEntry::setActive(bool active) {
    this->active = active;
    return *this;
}

nlohmann::json KeyStoreWrapEntry::getJson() {
    nlohmann::json json = {
            {Constants::NETWORK_KEY_HASH, Botan::hex_encode(this->hash,true)},
            {Constants::NETWORK_KEY_ACTIVE, this->active}
    };
    return json;
}

void KeyStoreWrapEntry::setKey(const keto::proto::NetworkKey& networkKey) {
    keto::rpc_protocol::NetworkKeyHelper networkKeyHelper(networkKey);

    Botan::DataSource_Memory datasource(networkKeyHelper.getKeyBytes_locked());
    std::shared_ptr<Botan::Private_Key> privateKey = Botan::PKCS8::load_key(datasource);

    keto::crypto::KeyBuilder keyBuilder;
    keyBuilder.addPrivateKey(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey())
            .addPrivateKey(privateKey);

    this->derivedKey =
            keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr(
                    new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(keyBuilder.getPrivateKey()));
}

keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr KeyStoreWrapEntry::getDerivedKey() {
    return this->derivedKey;
}


}
}