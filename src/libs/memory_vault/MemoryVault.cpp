

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

    keto::crypto::SecureVector key = keto::crypto::SecureVectorUtils().copyToSecure(
            this->keyStoragePtr->getEntry(id));
    this->memoryVaultPasswordEncryptorPtr->decrypt(password,key);

    keto::crypto::SecureVector valueCopy = keto::crypto::SecureVectorUtils().copyToSecure(
            this->storagePtr->getEntry(id));
    this->encryptorPtr->decrypt(key,valueCopy);
        return valueCopy;
}

void MemoryVault::removeValue(const keto::crypto::SecureVector& id) {
    this->storagePtr->removeEntry(id);
}



};
};
