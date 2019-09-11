//
// Created by Brett Chaldecott on 2019/01/15.
//

#include <keto/crypto/HashGenerator.hpp>
#include <keto/crypto/SecureVectorUtils.hpp>
#include "keto/memory_vault_session/MemoryVaultSessionKeyWrapper.hpp"
#include "keto/memory_vault_session/MemoryVaultSession.hpp"


#include <botan/pk_keys.h>
#include <botan/x509_key.h>

namespace keto {
namespace memory_vault_session {

std::string MemoryVaultSessionKeyWrapper::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVaultSessionKeyWrapper::MemoryVaultSessionKeyWrapper(const std::shared_ptr<Botan::Private_Key>& privateKey) {
    //KETO_LOG_DEBUG << "Retrieve the bytes for the key";
    keto::crypto::SecureVector bytes = Botan::PKCS8::BER_encode(*privateKey);
    //KETO_LOG_DEBUG << "Generate hash for bytes";
    this->hashId = keto::crypto::HashGenerator().generateHash(bytes);
    //KETO_LOG_DEBUG << "Create the entry in the vault";
    this->memoryVaultSessionEntryPtr = MemoryVaultSession::getInstance()->createEntry(
            keto::crypto::SecureVectorUtils().copySecureToString(hashId),bytes);
    //KETO_LOG_DEBUG << "complete";
}

MemoryVaultSessionKeyWrapper::MemoryVaultSessionKeyWrapper(const std::shared_ptr<Botan::Public_Key>& publicKey) {
    keto::crypto::SecureVector bytes = keto::crypto::SecureVectorUtils().copyToSecure(
            publicKey->public_key_bits());
    this->hashId = keto::crypto::HashGenerator().generateHash(bytes);
    this->memoryVaultSessionEntryPtr = MemoryVaultSession::getInstance()->createEntry(
            keto::crypto::SecureVectorUtils().copySecureToString(hashId),bytes);
}

MemoryVaultSessionKeyWrapper::~MemoryVaultSessionKeyWrapper() {
    MemoryVaultSession::getInstance()->removeEntry(keto::crypto::SecureVectorUtils().copySecureToString(hashId));
    this->memoryVaultSessionEntryPtr.reset();
}


std::shared_ptr<Botan::Private_Key> MemoryVaultSessionKeyWrapper::getPrivateKey() {


    keto::crypto::SecureVector bytes = this->memoryVaultSessionEntryPtr->getValue();
    Botan::DataSource_Memory memoryDatasource(bytes);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);
    return privateKey;
}

MemoryVaultSessionKeyWrapper& MemoryVaultSessionKeyWrapper::operator = (const std::shared_ptr<Botan::Private_Key>& key) {
    keto::crypto::SecureVector bytes = Botan::PKCS8::BER_encode(*key);
    this->memoryVaultSessionEntryPtr->setValue(bytes);
    return *this;
}

std::shared_ptr<Botan::Public_Key> MemoryVaultSessionKeyWrapper::getPublicKey() {
    keto::crypto::SecureVector bytes = this->memoryVaultSessionEntryPtr->getValue();
    return std::shared_ptr<Botan::Public_Key>(
            Botan::X509::load_key(keto::crypto::SecureVectorUtils().copyFromSecure(bytes)));
}

MemoryVaultSessionKeyWrapper& MemoryVaultSessionKeyWrapper::operator = (const std::shared_ptr<Botan::Public_Key>& key) {
    keto::crypto::SecureVector bytes = keto::crypto::SecureVectorUtils().copyToSecure(
            key->public_key_bits());
    this->memoryVaultSessionEntryPtr->setValue(bytes);
    return *this;
}

MemoryVaultSessionKeyWrapper::operator std::shared_ptr<Botan::Private_Key>() {
    keto::crypto::SecureVector bytes = this->memoryVaultSessionEntryPtr->getValue();
    Botan::DataSource_Memory memoryDatasource(bytes);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);
    return privateKey;
}

MemoryVaultSessionKeyWrapper::operator std::shared_ptr<Botan::Public_Key>() {
    keto::crypto::SecureVector bytes = this->memoryVaultSessionEntryPtr->getValue();
    return std::shared_ptr<Botan::Public_Key>(
            Botan::X509::load_key(keto::crypto::SecureVectorUtils().copyFromSecure(bytes)));

}


}
}
