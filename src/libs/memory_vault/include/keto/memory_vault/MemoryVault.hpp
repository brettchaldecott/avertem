/**
 *
 */

#ifndef KETO_MEMORY_VAULT_HPP
#define KETO_MEMORY_VAULT_HPP

#include <memory>
#include <string>
#include <vector>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"

#include "keto/memory_vault/MemoryVaultEncryptor.hpp"
#include "keto/memory_vault/MemoryVaultStorage.hpp"
#include "keto/memory_vault/MemoryVaultPasswordEncryptor.hpp"

namespace keto {
namespace memory_vault {

class MemoryVault;
typedef std::shared_ptr<MemoryVault> MemoryVaultPtr;

class MemoryVault {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVault(uint8_t slotId, const keto::crypto::SecureVector& hashId, const MemoryVaultEncryptorPtr encryptorPtr);
    MemoryVault(const MemoryVault& orig) = delete;
    virtual ~MemoryVault();

    uint8_t getSlot();
    keto::crypto::SecureVector getHashId();

    void setValue(const keto::crypto::SecureVector& id,
                                        const keto::crypto::SecureVector& password,
                                        const keto::crypto::SecureVector& value);

    keto::crypto::SecureVector getValue(const keto::crypto::SecureVector& id,
                                        const keto::crypto::SecureVector& password);

    void removeValue(const keto::crypto::SecureVector& id);


private:
    uint8_t slotId;
    const keto::crypto::SecureVector hashId;
    MemoryVaultEncryptorPtr encryptorPtr;
    MemoryVaultPasswordEncryptorPtr memoryVaultPasswordEncryptorPtr;
    MemoryVaultStoragePtr keyStoragePtr;
    MemoryVaultStoragePtr storagePtr;


};

}
}


#endif