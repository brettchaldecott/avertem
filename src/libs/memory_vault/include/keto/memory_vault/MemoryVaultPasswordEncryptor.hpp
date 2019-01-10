//
// Created by Brett Chaldecott on 2019/01/10.
//

#ifndef KETO_MEMORYVAULTPASSWORDENCRYPTOR_HPP
#define KETO_MEMORYVAULTPASSWORDENCRYPTOR_HPP

#include <memory>
#include <string>
#include <vector>

#include "keto/crypto/PasswordPipeLine.hpp"

namespace keto {
namespace memory_vault {

class MemoryVaultPasswordEncryptor;
typedef std::shared_ptr<MemoryVaultPasswordEncryptor> MemoryVaultPasswordEncryptorPtr;

class MemoryVaultPasswordEncryptor {
public:
    MemoryVaultPasswordEncryptor();
    MemoryVaultPasswordEncryptor(const MemoryVaultPasswordEncryptor& orig) = delete;
    virtual ~MemoryVaultPasswordEncryptor();

    void encrypt(const keto::crypto::SecureVector& password, keto::crypto::SecureVector& value);
    void decrypt(const keto::crypto::SecureVector& password, keto::crypto::SecureVector& value);

private:
    keto::crypto::PasswordPipeLinePtr passwordPipeLinePtr;
};

}
}


#endif //KETO_MEMORYVAULTPASSWORDENCRYPTOR_HPP
