/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#define BOOST_TEST_MODULE SoftwareConsensusTest

#pragma warning(disable: 4503)

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

#include "keto/software_consensus/Constants.hpp"
#include <chaiscript/chaiscript.hpp>

std::string helloWorld(const std::string &t_name) {
  return "Hello " + t_name + "!";
}

BOOST_AUTO_TEST_CASE( hash_builder_test ) {
    
    std::cout << "The obfuscated string : " << DEF_OBFUSCATED(R"(cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $
            this is
            and now)").decrypt() << std::endl;
    std::cout << "The version : " << keto::software_consensus::Constants::getHeaderVersion() << std::endl;
    std::cout << "The source version : " << keto::software_consensus::Constants::getSourceVersion() << std::endl;
    
    chaiscript::ChaiScript chai;
  chai.add(chaiscript::fun(&helloWorld), "helloWorld");

  chai.eval(R"(
    print(helloWorld("Bob"));
  )");
    
}