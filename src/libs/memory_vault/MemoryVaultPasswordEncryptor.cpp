//
// Created by Brett Chaldecott on 2019/01/10.
//


#include <botan/block_cipher.h>
#include <botan/stream_cipher.h>

#include "keto/memory_vault/MemoryVaultPasswordEncryptor.hpp"


namespace keto {
namespace memory_vault {

MemoryVaultPasswordEncryptor::MemoryVaultPasswordEncryptor() {
    this->passwordPipeLinePtr = keto::crypto::PasswordPipeLinePtr(new keto::crypto::PasswordPipeLine());
}

MemoryVaultPasswordEncryptor::~MemoryVaultPasswordEncryptor() {

}

void MemoryVaultPasswordEncryptor::encrypt(const keto::crypto::SecureVector& password, keto::crypto::SecureVector& value) {
    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create("ChaCha(20)");

    cipher->set_key(Botan::SymmetricKey(this->passwordPipeLinePtr->generatePassword(password)));
    cipher->set_iv(NULL,0);

    cipher->encrypt(value);
}

void MemoryVaultPasswordEncryptor::decrypt(const keto::crypto::SecureVector& password, keto::crypto::SecureVector& value) {
    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create("ChaCha(20)");

    cipher->set_key(Botan::SymmetricKey(this->passwordPipeLinePtr->generatePassword(password)));
    cipher->set_iv(NULL,0);

    cipher->decrypt(value);
}


}
}