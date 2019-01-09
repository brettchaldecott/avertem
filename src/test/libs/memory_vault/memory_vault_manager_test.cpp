//
// Created by Brett Chaldecott on 2019/01/08.
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
#include "keto/memory_vault/MemoryVaultManager.hpp"
#include "keto/memory_vault/Exception.hpp"

BOOST_AUTO_TEST_CASE( memory_vault_manager_test ) {

    keto::memory_vault::MemoryVaultManagerPtr memoryVaultManagerPtr = keto::memory_vault::MemoryVaultManager::init(); // = keto::memory_vault::MemoryVaultManager::

    keto::memory_vault::vectorOfSecureVectors sessions;
    sessions.push_back(keto::crypto::SecureVectorUtils().copyStringToSecure("test"));

    memoryVaultManagerPtr->createSession(sessions);
    keto::memory_vault::MemoryVaultPtr memoryVaultPtr =
            memoryVaultManagerPtr->createVault("test",keto::crypto::SecureVectorUtils().copyStringToSecure("test"),keto::crypto::SecureVectorUtils().copyStringToSecure("test"));

    memoryVaultPtr = memoryVaultManagerPtr->getVault("test",keto::crypto::SecureVectorUtils().copyStringToSecure("test"));

    keto::crypto::SecureVector testValue = keto::crypto::SecureVectorUtils().copyStringToSecure("here be dragons");
    keto::crypto::SecureVector key = memoryVaultPtr->setValue(keto::crypto::SecureVectorUtils().copyStringToSecure("test"),testValue);

    keto::crypto::SecureVector value = memoryVaultPtr->getValue(keto::crypto::SecureVectorUtils().copyStringToSecure("test"),key);

    BOOST_CHECK_EQUAL_COLLECTIONS(value.begin(),value.end(),testValue.begin(),testValue.end());

    memoryVaultManagerPtr->clearSession();

    BOOST_REQUIRE_THROW(memoryVaultManagerPtr->getVault("test",keto::crypto::SecureVectorUtils().copyStringToSecure("test")), keto::memory_vault::UnknownVaultException);

    keto::memory_vault::MemoryVaultManager::fin();

}