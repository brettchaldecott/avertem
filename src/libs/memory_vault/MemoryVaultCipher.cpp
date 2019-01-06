//
// Created by Brett Chaldecott on 2018/12/30.
//

#include "keto/memory_vault/MemoryVaultCipher.hpp"

#include <botan/block_cipher.h>
#include <botan/auto_rng.h>
#include <botan/bigint.h>


namespace keto {
namespace memory_vault {


std::string MemoryVaultCipher::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVaultCipher::MemoryVaultCipher() {
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create("ChaCha(20)");
    this->secureVector = generator->random_vec(cipher->key_spec().maximum_keylength());

}

MemoryVaultCipher::~MemoryVaultCipher() {

}

MemoryVaultCipher& MemoryVaultCipher::setId(const bytes& id) {
    this->id = id;
    return *this;
}

bytes MemoryVaultCipher::getId() {
    return this->id;
}

void MemoryVaultCipher::encrypt(keto::crypto::SecureVector& value) {
    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create("ChaCha(20)");

    cipher->set_key(Botan::SymmetricKey(this->secureVector));
    cipher->set_iv(NULL,0);

    cipher->encrypt(value);
}

void MemoryVaultCipher::decrypt(keto::crypto::SecureVector& value) {
    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create("ChaCha(20)");

    cipher->set_key(Botan::SymmetricKey(this->secureVector));
    cipher->set_iv(NULL,0);

    cipher->decrypt(value);

}


}
}