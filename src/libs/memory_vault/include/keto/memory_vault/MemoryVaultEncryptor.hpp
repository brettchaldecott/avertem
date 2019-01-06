//
// Created by Brett Chaldecott on 2018/12/31.
//

#ifndef KETO_MEMORYVAULTENCRYPTOR_HPP
#define KETO_MEMORYVAULTENCRYPTOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>

#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/dh.h>
#include <botan/elgamal.h>
#include <botan/bigint.h>
#include <botan/numthry.h>

#include "keto/memory_vault/MemoryVaultCipher.hpp"
#include "keto/crypto/Containers.hpp"


namespace keto {
namespace memory_vault {

class MemoryVaultEncryptor;
typedef std::shared_ptr<MemoryVaultEncryptor> MemoryVaultEncryptorPtr;

class MemoryVaultEncryptor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVaultEncryptor();
    MemoryVaultEncryptor(const MemoryVaultEncryptor& orig) = delete;
    virtual ~MemoryVaultEncryptor();


    keto::crypto::SecureVector encrypt(keto::crypto::SecureVector& value);
    void decrypt(const keto::crypto::SecureVector& key, keto::crypto::SecureVector& value);

private:
    size_t iterations;
    size_t bits;
    Botan::BigInt modulas;
    std::vector<MemoryVaultCipherPtr> ciphers;
    std::map<bytes,MemoryVaultCipherPtr> bytesCiphers;

};


}
}

#endif //KETO_MEMORYVAULTENCRYPTOR_HPP
