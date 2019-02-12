//
// Created by Brett Chaldecott on 2018/12/31.
//

#include <random>
#include <algorithm>
#include <botan/hex.h>

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

    //std::cout << "The bits are : " << this->bits * 8 << std::endl;
    for (int index = 0; index < (this->bits * 8); index++) {
        MemoryVaultCipherPtr memoryVaultCipherPtr(new MemoryVaultCipher);
        this->ciphers.push_back(memoryVaultCipherPtr);
        bytes randomId;
        do {
            Botan::BigInt randomNum(generator->random_vec(this->bits * 8));
            randomNum = Botan::power_mod(randomNum, index,twoBytesModulas);
            randomId = Botan::BigInt::encode(randomNum);
            //std::cout << "The random id length : " << randomId.size() << std::endl;
            if (randomId.size() == 1) {
                randomId.insert(randomId.begin(),randomId.begin(),randomId.end());
            }
            //std::cout << "The random id length : " << randomId.size() << std::endl;
        }
        while ( randomId.size() == 0 || this->bytesCiphers.count(randomId));
        std::cout << "The random id length : " << randomId.size() << std::endl;
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
    std::uniform_int_distribution<int> distribution(0,this->ciphers.size());
    distribution(stdGenerator);

    int currentPos = -1;
    //std::cout << "loop through the iterations : " << this->iterations << std::endl;
    //std::cout << "Hash size is : " << hash.size() << std::endl;
    for (int index = 0; index < this->iterations; index++) {
        //std::cout << "index : " << index << std::endl;
        int randomPos;
        do {
            randomPos = distribution(stdGenerator);
        } while (currentPos == randomPos);
        MemoryVaultCipherPtr memoryVaultCipherPtr = this->ciphers[randomPos];
        keto::crypto::SecureVector id = keto::crypto::SecureVectorUtils().copyToSecure(
                memoryVaultCipherPtr->getId());
        // encrypt the values
        //std::cout << "Value : " << Botan::hex_encode(value) << std::endl;
        memoryVaultCipherPtr->encrypt(value);
        //std::cout << "Value : " << Botan::hex_encode(value) << std::endl;
        memoryVaultCipherPtr->encrypt(hash);

        // increment the key
        //std::cout << "The random pos : " << randomPos << std::endl;
        //std::cout << "The id size : " << id.size() << std::endl;
        //std::cout << "The id is : " << Botan::hex_encode(id) << std::endl;
        //std::cout << "The id is size : " << memoryVaultCipherPtr->getId().size() << std::endl;
        keto::crypto::SecureVector vector = keto::crypto::SecureVectorUtils().bitwiseXor(id,
                keto::crypto::SecureVector(&hash[index*2],&hash[(index*2)+2]));
        result.insert(result.end(),vector.begin(),vector.end());
        //std::cout << "The result size is : " << result.size() << std::endl;
        //std::cout << "The hex encode : " << Botan::hex_encode(result) << std::endl;
        currentPos = randomPos;
    }
    //std::cout << "Value [" << hash.size() << "] value : " << Botan::hex_encode(hash) << std::endl;
    value.insert(value.begin(),hash.begin(),hash.end());

    return result;
}

void MemoryVaultEncryptor::decrypt(const keto::crypto::SecureVector& key, keto::crypto::SecureVector& value) {
    //std::cout << "The size of the hash is : " << this->iterations*2 << std::endl;
    keto::crypto::SecureVector encryptedHash(&value[0],&value[32]);
    keto::crypto::SecureVector encryptedValue(&value[32],&value[value.size()]);

    //std::cout << "Value [" << encryptedHash.size() << "] value : " << Botan::hex_encode(encryptedHash) << std::endl;

    for (int index = this->iterations -1; index >= 0; index--) {
        //std::cout << "Iterations : " << index << std::endl;

        bytes entryId = keto::crypto::SecureVectorUtils().copyFromSecure(
                keto::crypto::SecureVectorUtils().bitwiseXor(
                keto::crypto::SecureVector(&key[index*2],&key[(index*2)+2]),
                keto::crypto::SecureVector(&encryptedHash[index*2],&encryptedHash[(index*2)+2])));
        //std::cout << "The id is : " << Botan::hex_encode(entryId) << std::endl;
        MemoryVaultCipherPtr memoryVaultCipherPtr = this->bytesCiphers[entryId];
        if (!memoryVaultCipherPtr) {
            BOOST_THROW_EXCEPTION(InvalidCipherIDException());
        }

        //std::cout << "Size of bytes : " << entryId.size() << std::endl;

        // decrypt
        //std::cout << "Hash code : " << Botan::hex_encode(encryptedHash) << std::endl;
        memoryVaultCipherPtr->decrypt(encryptedHash);
        //std::cout << "Decrypt the value : " << Botan::hex_encode(encryptedValue) << std::endl;
        memoryVaultCipherPtr->decrypt(encryptedValue);
        //std::cout << "After the decryption : " << Botan::hex_encode(encryptedValue) << std::endl;
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