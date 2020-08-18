//
// Created by Brett Chaldecott on 2018/12/31.
//

#include <random>
#include <algorithm>
#include <botan/hex.h>
#include <sstream>

#include "keto/memory_vault/MemoryVaultEncryptor.hpp"
#include "keto/memory_vault/Exception.hpp"

#include "keto/memory_vault/Constants.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/crypto/HashGenerator.hpp"

namespace keto {
namespace memory_vault {

std::string MemoryVaultEncryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


MemoryVaultEncryptor::MemoryVaultEncryptor() {
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(1,9);
    distribution(stdGenerator);

    this->iterations = Constants::BASE_SIZE + (distribution(stdGenerator) % Constants::MODULAS_SIZE);
    this->bits = this->iterations * 8;
    this->modulas = Botan::BigInt::power_of_2(this->bits);

    Botan::BigInt twoBytesModulas = Botan::BigInt::power_of_2(Constants::ITERATION_BITS_SIZE);

    for (int index = 0; index < (this->bits * 8); index++) {
        MemoryVaultCipherPtr memoryVaultCipherPtr(new MemoryVaultCipher);
        this->ciphers.push_back(memoryVaultCipherPtr);
        bytes randomId;
        do {
            Botan::BigInt randomNum(generator->random_vec(this->bits * 8));
            randomNum = Botan::power_mod(randomNum, index,twoBytesModulas);
            randomId = Botan::BigInt::encode(randomNum);
            if (randomId.size() == 1) {
                randomId.insert(randomId.begin(),randomId.begin(),randomId.end());
            }
        }
        while ( randomId.size() == 0 || this->bytesCiphers.count(randomId));
        memoryVaultCipherPtr->setId(randomId);
        this->bytesCiphers[randomId] = memoryVaultCipherPtr;
    }
}

MemoryVaultEncryptor::~MemoryVaultEncryptor() {

}


keto::crypto::SecureVector MemoryVaultEncryptor::encrypt(keto::crypto::SecureVector& value) {
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    keto::crypto::HashGenerator hashGenerator;
    keto::crypto::SecureVector hash = hashGenerator.generateHash(value);//buildCheckHash(hashGenerator.generateHash(value),this->iterations);

    keto::crypto::SecureVector result;
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,this->ciphers.size()-1);
    distribution(stdGenerator);

    int currentPos = -1;
    for (int index = 0; index < this->iterations; index++) {
        int randomPos;
        do {
            randomPos = distribution(stdGenerator);
        } while (currentPos == randomPos);
        MemoryVaultCipherPtr memoryVaultCipherPtr = this->ciphers[randomPos];
        keto::crypto::SecureVector id = keto::crypto::SecureVectorUtils().copyToSecure(
                memoryVaultCipherPtr->getId());
        // encrypt the values
        memoryVaultCipherPtr->encrypt(value);
        memoryVaultCipherPtr->encrypt(hash);

        // increment the key
        keto::crypto::SecureVector vector = keto::crypto::SecureVectorUtils().bitwiseXor(id,
                keto::crypto::SecureVector(&hash[index*2],&hash[(index*2)+2]));
        result.insert(result.end(),vector.begin(),vector.end());
        currentPos = randomPos;
    }
    value.insert(value.begin(),hash.begin(),hash.end());

    return result;
}

void MemoryVaultEncryptor::decrypt(const keto::crypto::SecureVector& key, keto::crypto::SecureVector& value) {
    keto::crypto::SecureVector encryptedHash(&value[0],&value[32]);
    keto::crypto::SecureVector encryptedValue(&value[32],&value[value.size()]);


    for (int index = this->iterations -1; index >= 0; index--) {

        bytes entryId = keto::crypto::SecureVectorUtils().copyFromSecure(
                keto::crypto::SecureVectorUtils().bitwiseXor(
                keto::crypto::SecureVector(&key[index*2],&key[(index*2)+2]),
                keto::crypto::SecureVector(&encryptedHash[index*2],&encryptedHash[(index*2)+2])));
        MemoryVaultCipherPtr memoryVaultCipherPtr = this->bytesCiphers[entryId];
        if (!memoryVaultCipherPtr) {
            std::stringstream ss;
            ss << "[InvalidCipherIDException] Failed to find the entry id [" << Botan::hex_encode(entryId,true) << "]";
            BOOST_THROW_EXCEPTION(InvalidCipherIDException(ss.str()));
        }


        // decrypt
        memoryVaultCipherPtr->decrypt(encryptedHash);
        memoryVaultCipherPtr->decrypt(encryptedValue);
    }
    keto::crypto::HashGenerator hashGenerator;
    keto::crypto::SecureVector hash = hashGenerator.generateHash(encryptedValue);
    if (!std::equal(&encryptedHash[0],&encryptedHash[hash.size() -1], hash.begin())) {
        BOOST_THROW_EXCEPTION(DecryptionFailedException());
    }

    value = encryptedValue;
}


}
}
