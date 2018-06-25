/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#define BOOST_TEST_MODULE KeyToolsTest

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <botan/hash.h>
#include <botan/rsa.h>
#include <botan/rng.h>
#include <botan/p11_randomgenerator.h>
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>

#include <boost/test/unit_test.hpp>
#include <iostream>

#include <keto/key_tools/KeyPairCreator.hpp>
#include <keto/key_tools/ContentEncryptor.hpp>
#include <keto/key_tools/ContentDecryptor.hpp>

BOOST_AUTO_TEST_CASE( key_tools_test ) {
    
    
}