//
// Created by Brett Chaldecott on 2018/12/30.
//

#ifndef KETO_MEMORYVAULTCIPHER_HPP
#define KETO_MEMORYVAULTCIPHER_HPP

#include <memory>
#include <string>

#include <botan/block_cipher.h>
#include <botan/stream_cipher.h>

#include "keto/crypto/Containers.hpp"

namespace keto {
namespace memory_vault {

class MemoryVaultCipher;
typedef std::shared_ptr<MemoryVaultCipher> MemoryVaultCipherPtr;
typedef std::vector<uint8_t> bytes;


class MemoryVaultCipher {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVaultCipher();
    MemoryVaultCipher(const MemoryVaultCipher& memoryVaultCipher) = delete;
    virtual ~MemoryVaultCipher();

    MemoryVaultCipher& setId(const bytes& id);
    bytes getId();

    void encrypt(keto::crypto::SecureVector& value);
    void decrypt(keto::crypto::SecureVector& value);

private:
    bytes id;
    keto::crypto::SecureVector secureVector;
    //std::unique_ptr<Botan::StreamCipher> cipher;
};


}
}


#endif //KETO_MEMORYVAULTCIPHER_HPP
