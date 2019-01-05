
#include "keto/memory_vault/MemoryVault.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace memory_vault {

std::string MemoryVault::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVault::MemoryVault(const keto::crypto::SecureVector& hashId, const MemoryVaultEncryptorPtr encryptorPtr) :
    hashId(hashId), encryptorPtr(encryptorPtr) {
    storagePtr = MemoryVaultStoragePtr(new MemoryVaultStorage());
}

MemoryVault::~MemoryVault() {

}

keto::crypto::SecureVector MemoryVault::getHashId() {
    return this->hashId;
}

keto::crypto::SecureVector MemoryVault::setValue(const keto::crypto::SecureVector& id,
                                    const keto::crypto::SecureVector& value) {
    keto::crypto::SecureVector valueCopy = value;
    keto::crypto::SecureVector key = this->encryptorPtr->encrypt(valueCopy);
    this->storagePtr->setEntry(id,keto::crypto::SecureVectorUtils().copyFromSecure(valueCopy));
    return key;
}

keto::crypto::SecureVector MemoryVault::getValue(const keto::crypto::SecureVector& id,
                                    const keto::crypto::SecureVector& key) {
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
