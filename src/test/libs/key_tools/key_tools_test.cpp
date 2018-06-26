/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#define BOOST_TEST_MODULE KeyToolsTest

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

#include "keto/key_tools/KeyPairCreator.hpp"
#include "keto/key_tools/ContentEncryptor.hpp"
#include "keto/key_tools/ContentDecryptor.hpp"

BOOST_AUTO_TEST_CASE( key_tools_test ) {
    
    std::vector<uint8_t> content({'t','e','s','t'});
    
    std::cout << "Create a key pair" << std::endl;
    keto::key_tools::KeyPairCreator keyPairCreator;
    std::cout << "Encrypt" << std::endl;
    keto::key_tools::ContentEncryptor contentEncryptor(keyPairCreator.getSecret(),
            keyPairCreator.getEncodedKey(),content);
    
    std::cout << "Decrypt" << std::endl;
    keto::key_tools::ContentDecryptor contentDecryptor(keyPairCreator.getSecret(),
            keyPairCreator.getEncodedKey(),contentEncryptor.getEncryptedContent());
    
    std::cout << "Check results" << std::endl;
    if (keto::crypto::SecureVectorUtils().copyFromSecure(contentDecryptor.getContent()) ==
            content) {
        std::cout << "The content is equal" << std::endl;
    } else {
        std::cout << "The content is not equal" << std::endl;
    }
}