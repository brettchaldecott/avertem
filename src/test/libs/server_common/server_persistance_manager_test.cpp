//
// Created by Brett Chaldecott on 2019/01/09.
//

#define BOOST_TEST_MODULE KeyToolsTest

#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <string>
#include <iostream>
#include <stdlib.h>

#include <botan/hash.h>
#include <botan/rsa.h>
#include <botan/rng.h>
#include <botan/p11_randomgenerator.h>
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/test/unit_test.hpp>
#include <iostream>

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/crypto/ConfigurableSha256.hpp"
#include "keto/crypto/ShaDigest.hpp"

#include "keto/server_common/StatePersistanceManager.hpp"

static const char* KETO_HOME = "KETO_HOME=/tmp/";

BOOST_AUTO_TEST_CASE( configurable_sha256_test ) {
    putenv((char*)KETO_HOME);
    std::cout << "The state test" << std::endl;
    keto::server_common::StatePersistanceManagerPtr statePersistanceManagerPtr =
            keto::server_common::StatePersistanceManager::init("/tmp/test.ini");
    std::cout << "Set the value" << std::endl;
    (*statePersistanceManagerPtr)["fred"].set("bob");
    std::cout << "fred : " << (std::string)(*statePersistanceManagerPtr)["fred"] << std::endl;
    std::cout << "Persist the value" << std::endl;
    statePersistanceManagerPtr->persist();
    std::cout << "Reset and clean up memory" << std::endl;
    statePersistanceManagerPtr.reset();
    keto::server_common::StatePersistanceManager::fin();
    std::cout << "The state test" << std::endl;

    keto::server_common::StatePersistanceManagerPtr statePersistanceManagerPtr2 =
            keto::server_common::StatePersistanceManager::init("/tmp/test.ini");
    std::cout << "fred : " << (std::string)(*statePersistanceManagerPtr2)["fred"] << std::endl;

    BOOST_CHECK_EQUAL("bob",(const std::string)(*statePersistanceManagerPtr2)["fred"]);

    boost::filesystem::path tmpPath = statePersistanceManagerPtr2->getStateFilePath();
    statePersistanceManagerPtr2.reset();
    keto::server_common::StatePersistanceManager::fin();

    boost::filesystem::remove(tmpPath);


}