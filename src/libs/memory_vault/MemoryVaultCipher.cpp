//
// Created by Brett Chaldecott on 2018/12/30.
//

#include "keto/memory_vault/MemoryVaultCipher.hpp"

#include <botan/block_cipher.h>
#include <botan/auto_rng.h>


namespace keto {
namespace memory_vault {


std::string MemoryVaultCipher::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVaultCipher::MemoryVaultCipher() {
    this->cipher = Botan::BlockCipher::create_or_throw("AES-256");
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    this->cipher->set_key(generator->random_vec(this->cipher->block_size()));
}

MemoryVaultCipher:: ~MemoryVaultCipher() {

}

MemoryVaultCipher& MemoryVaultCipher::setId(const bytes& id) {
    this->id = id;
    return *this;
}

bytes MemoryVaultCipher::getId() {
    return this->id;
}

void MemoryVaultCipher::encrypt(keto::crypto::SecureVector& value) {
    this->cipher->encrypt(value);
}

void MemoryVaultCipher::decrypt(keto::crypto::SecureVector& value) {
    this->cipher->decrypt(value);
}


}
}