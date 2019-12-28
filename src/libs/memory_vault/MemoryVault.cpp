

#include <botan/hex.h>

#include "keto/memory_vault/MemoryVault.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace memory_vault {

std::string MemoryVault::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVault::MemoryVault(uint8_t slotId, const keto::crypto::SecureVector& hashId, const MemoryVaultEncryptorPtr encryptorPtr) :
        slotId(slotId), hashId(hashId), encryptorPtr(encryptorPtr) {
    memoryVaultPasswordEncryptorPtr = MemoryVaultPasswordEncryptorPtr(new MemoryVaultPasswordEncryptor());
    storagePtr = MemoryVaultStoragePtr(new MemoryVaultStorage());
    keyStoragePtr = MemoryVaultStoragePtr(new MemoryVaultStorage());
}

MemoryVault::~MemoryVault() {

}

uint8_t MemoryVault::getSlot() {
    return this->slotId;
}

keto::crypto::SecureVector MemoryVault::getHashId() {
    return this->hashId;
}

void MemoryVault::setValue(const keto::crypto::SecureVector& id,
                                                 const keto::crypto::SecureVector& password,
                                                 const keto::crypto::SecureVector& value) {
    keto::crypto::SecureVector valueCopy = value;
    keto::crypto::SecureVector key = this->encryptorPtr->encrypt(valueCopy);
    this->storagePtr->setEntry(id,keto::crypto::SecureVectorUtils().copyFromSecure(valueCopy));

    this->memoryVaultPasswordEncryptorPtr->encrypt(password,key);

    this->keyStoragePtr->setEntry(id,keto::crypto::SecureVectorUtils().copyFromSecure(key));
}

keto::crypto::SecureVector MemoryVault::getValue(const keto::crypto::SecureVector& id,
                                    const keto::crypto::SecureVector& password) {

    //auto start = std::chrono::steady_clock::now();

    //KETO_LOG_DEBUG << "[MemoryVault::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "]get the key";

    keto::crypto::SecureVector key = keto::crypto::SecureVectorUtils().copyToSecure(
            this->keyStoragePtr->getEntry(id));
    this->memoryVaultPasswordEncryptorPtr->decrypt(password,key);

    //KETO_LOG_DEBUG << "[MemoryVault::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "]get the value";
    keto::crypto::SecureVector valueCopy = keto::crypto::SecureVectorUtils().copyToSecure(
            this->storagePtr->getEntry(id));
    //KETO_LOG_DEBUG << "[MemoryVault::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "]decrypt the key";
    this->encryptorPtr->decrypt(key,valueCopy);
    //KETO_LOG_DEBUG << "[MemoryVault::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "]return the value";
    return valueCopy;
}

void MemoryVault::removeValue(const keto::crypto::SecureVector& id) {
    this->storagePtr->removeEntry(id);
}



};
};
