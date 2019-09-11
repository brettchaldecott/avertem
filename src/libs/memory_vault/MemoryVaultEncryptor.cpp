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

    //KETO_LOG_DEBUG << "The bits are : " << this->bits * 8;
    for (int index = 0; index < (this->bits * 8); index++) {
        MemoryVaultCipherPtr memoryVaultCipherPtr(new MemoryVaultCipher);
        this->ciphers.push_back(memoryVaultCipherPtr);
        bytes randomId;
        do {
            Botan::BigInt randomNum(generator->random_vec(this->bits * 8));
            randomNum = Botan::power_mod(randomNum, index,twoBytesModulas);
            randomId = Botan::BigInt::encode(randomNum);
            //KETO_LOG_DEBUG << "The random id length : " << randomId.size();
            if (randomId.size() == 1) {
                randomId.insert(randomId.begin(),randomId.begin(),randomId.end());
            }
            //KETO_LOG_DEBUG << "The random id length : " << randomId.size();
        }
        while ( randomId.size() == 0 || this->bytesCiphers.count(randomId));
        //KETO_LOG_DEBUG << "The random id length : " << randomId.size();
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
    //KETO_LOG_DEBUG << "loop through the iterations : " << this->iterations;
    //KETO_LOG_DEBUG << "Hash size is : " << hash.size();
    for (int index = 0; index < this->iterations; index++) {
        //KETO_LOG_DEBUG << "index : " << index;
        int randomPos;
        do {
            randomPos = distribution(stdGenerator);
        } while (currentPos == randomPos);
        MemoryVaultCipherPtr memoryVaultCipherPtr = this->ciphers[randomPos];
        keto::crypto::SecureVector id = keto::crypto::SecureVectorUtils().copyToSecure(
                memoryVaultCipherPtr->getId());
        // encrypt the values
        //KETO_LOG_DEBUG << "Value : " << Botan::hex_encode(value);
        memoryVaultCipherPtr->encrypt(value);
        //KETO_LOG_DEBUG << "Value : " << Botan::hex_encode(value);
        memoryVaultCipherPtr->encrypt(hash);

        // increment the key
        //KETO_LOG_DEBUG << "The random pos : " << randomPos;
        //KETO_LOG_DEBUG << "The id size : " << id.size();
        //KETO_LOG_DEBUG << "The id is : " << Botan::hex_encode(id);
        //KETO_LOG_DEBUG << "The id is size : " << memoryVaultCipherPtr->getId().size();
        keto::crypto::SecureVector vector = keto::crypto::SecureVectorUtils().bitwiseXor(id,
                keto::crypto::SecureVector(&hash[index*2],&hash[(index*2)+2]));
        result.insert(result.end(),vector.begin(),vector.end());
        //KETO_LOG_DEBUG << "The result size is : " << result.size();
        //KETO_LOG_DEBUG << "The hex encode : " << Botan::hex_encode(result);
        currentPos = randomPos;
    }
    //KETO_LOG_DEBUG << "Value [" << hash.size() << "] value : " << Botan::hex_encode(hash);
    value.insert(value.begin(),hash.begin(),hash.end());

    return result;
}

void MemoryVaultEncryptor::decrypt(const keto::crypto::SecureVector& key, keto::crypto::SecureVector& value) {
    //KETO_LOG_DEBUG << "The size of the hash is : " << this->iterations*2;
    keto::crypto::SecureVector encryptedHash(&value[0],&value[32]);
    keto::crypto::SecureVector encryptedValue(&value[32],&value[value.size()]);

    //KETO_LOG_DEBUG << "Value [" << encryptedHash.size() << "] value : " << Botan::hex_encode(encryptedHash);

    for (int index = this->iterations -1; index >= 0; index--) {
        //KETO_LOG_DEBUG << "Iterations : " << index;

        bytes entryId = keto::crypto::SecureVectorUtils().copyFromSecure(
                keto::crypto::SecureVectorUtils().bitwiseXor(
                keto::crypto::SecureVector(&key[index*2],&key[(index*2)+2]),
                keto::crypto::SecureVector(&encryptedHash[index*2],&encryptedHash[(index*2)+2])));
        //KETO_LOG_DEBUG << "The id is : " << Botan::hex_encode(entryId);
        MemoryVaultCipherPtr memoryVaultCipherPtr = this->bytesCiphers[entryId];
        if (!memoryVaultCipherPtr) {
            std::stringstream ss;
            ss << "[InvalidCipherIDException] Failed to find the entry id [" << Botan::hex_encode(entryId,true) << "]";
            BOOST_THROW_EXCEPTION(InvalidCipherIDException(ss.str()));
        }

        //KETO_LOG_DEBUG << "Size of bytes : " << entryId.size();

        // decrypt
        //KETO_LOG_DEBUG << "Hash code : " << Botan::hex_encode(encryptedHash);
        memoryVaultCipherPtr->decrypt(encryptedHash);
        //KETO_LOG_DEBUG << "Decrypt the value : " << Botan::hex_encode(encryptedValue);
        memoryVaultCipherPtr->decrypt(encryptedValue);
        //KETO_LOG_DEBUG << "After the decryption : " << Botan::hex_encode(encryptedValue);
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
