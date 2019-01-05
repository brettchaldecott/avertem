//
// Created by Brett Chaldecott on 2019/01/05.
//

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


}