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
#include "keto/crypto/PasswordPipeLine.hpp"


BOOST_AUTO_TEST_CASE( password_pipe_line_test ) {

    keto::crypto::PasswordPipeLine passwordPipeLine;

    keto::crypto::SecureVector hash1 = passwordPipeLine.generatePassword(keto::crypto::SecureVectorUtils().copyStringToSecure("bob"));
    keto::crypto::SecureVector hash2 = passwordPipeLine.generatePassword(keto::crypto::SecureVectorUtils().copyStringToSecure("bob"));


    std::cout << "Hash  : " << Botan::hex_encode(hash1) << std::endl;
    std::cout << "Hash  : " << Botan::hex_encode(hash2) << std::endl;

    BOOST_CHECK_EQUAL_COLLECTIONS(hash1.begin(),hash1.end(),hash2.begin(),hash2.end());

}