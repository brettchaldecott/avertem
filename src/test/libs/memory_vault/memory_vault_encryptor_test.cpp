//
// Created by Brett Chaldecott on 2019/01/05.
//

#define BOOST_TEST_MODULE MemoryVaultTest

#pragma warning(disable: 4503)

#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <string>
#include <iostream>

#include <botan/hash.h>
#include <botan/rsa.h>
#include <botan/rng.h>
#include <botan/p11_randomgenerator.h>
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>

#include <boost/test/unit_test.hpp>
#include <iostream>

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/memory_vault/MemoryVaultEncryptor.hpp"

BOOST_AUTO_TEST_CASE( memory_vault_encryptor_test ) {
    keto::memory_vault::MemoryVaultEncryptorPtr memoryVaultEncryptorPtr(new keto::memory_vault::MemoryVaultEncryptor());

    std::string value = "bob was here";
    keto::crypto::SecureVector secureVector(value.begin(),value.end());
    std::cout << "The encryption : " << secureVector.size() << std::endl;
    std::cout << "Hex for contents : " << Botan::hex_encode(secureVector) << std::endl;
    keto::crypto::SecureVector key = memoryVaultEncryptorPtr->encrypt(secureVector);
    std::cout << "The key : " << Botan::hex_encode(key) << std::endl;
    value = std::string(secureVector.begin(),secureVector.end());
    std::cout << "The value : " << value << std::endl;
    std::cout << "The value : " << Botan::hex_encode(secureVector) << std::endl;

    std::cout << "The decrypt : " << key.size() << std::endl;
    std::cout << "The secure vector : " << secureVector.size() << std::endl;
    std::cout << "Hex for contents : " << Botan::hex_encode(secureVector) << std::endl;
    memoryVaultEncryptorPtr->decrypt(key,secureVector);

    std::cout << "The string" << std::endl;
    value = std::string(secureVector.begin(),secureVector.end());
    std::cout << "The value : " << value << std::endl;
}