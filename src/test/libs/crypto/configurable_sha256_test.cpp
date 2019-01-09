//
// Created by Brett Chaldecott on 2019/01/09.
//

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
#include "keto/crypto/ConfigurableSha256.hpp"
#include "keto/crypto/ShaDigest.hpp"


BOOST_AUTO_TEST_CASE( configurable_sha256_test ) {

    keto::crypto::ShaDigest shaDigest;
    keto::crypto::ConfigurableSha256 configurableSha256(shaDigest.getDigest());

    std::shared_ptr<Botan::HashFunction> hash256 = Botan::HashFunction::create("SHA-256");

    keto::crypto::SecureVector baseHash1 = hash256->process(keto::crypto::SecureVectorUtils().copyStringToSecure("fred"));
    keto::crypto::SecureVector baseHash2 = hash256->process(keto::crypto::SecureVectorUtils().copyStringToSecure("fred"));
    keto::crypto::SecureVector hash1 = configurableSha256.process(keto::crypto::SecureVectorUtils().copyStringToSecure("fred"));
    //configurableSha256.clear();
    keto::crypto::SecureVector hash2 = configurableSha256.process(keto::crypto::SecureVectorUtils().copyStringToSecure("fred"));

    std::cout << "Hash  : " << Botan::hex_encode(baseHash1) << std::endl;
    std::cout << "Hash  : " << Botan::hex_encode(baseHash2) << std::endl;
    std::cout << "Hash1 : " << Botan::hex_encode(hash1) << std::endl;
    std::cout << "Hash2 : " << Botan::hex_encode(hash2) << std::endl;

    BOOST_CHECK_EQUAL_COLLECTIONS(hash1.begin(),hash1.end(),hash2.begin(),hash2.end());
}