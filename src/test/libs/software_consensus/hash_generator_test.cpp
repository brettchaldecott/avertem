/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#define BOOST_TEST_MODULE SoftwareHashGeneratorTest

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

#include "keto/obfuscate/MetaString.hpp"

#include "keto/crypto/Containers.hpp"
#include "keto/crypto/HashGenerator.hpp"

#include "keto/key_tools/KeyPairCreator.hpp"
#include "keto/key_tools/ContentEncryptor.hpp"

#include "keto/server_common/VectorUtils.hpp"

#include "keto/software_consensus/Constants.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"
#include "keto/software_consensus/hash_generator_test_config.hpp"


namespace keto {
namespace software_consensus {
namespace example_code1 {

keto::key_tools::KeyPairCreator keyPairCreator;
    
const char* CODE = R"(
    print("Bob");
    generateHash(getModuleSignature());
)";
    
keto::crypto::SecureVector getHash() {
    std::string code = CODE;
    return keto::crypto::HashGenerator().generateHash(
        keto::crypto::SecureVectorUtils().copyStringToSecure(code));
}

keto::crypto::SecureVector getEncodedKey() {
    return keyPairCreator.getEncodedKey();
}

std::vector<uint8_t> getCode() {
    std::string code = CODE;
    keto::key_tools::ContentEncryptor contentEncryptor(keyPairCreator.getSecret(), 
            keyPairCreator.getEncodedKey(),
            keto::server_common::VectorUtils().copyStringToVector(code));
    return contentEncryptor.getEncryptedContent();
}

}


namespace example_code2 {

keto::key_tools::KeyPairCreator keyPairCreator;
    
const char* CODE = R"(
    print("fred");
    generateHash(getModuleSignature());
)";
    
keto::crypto::SecureVector getHash() {
    std::string code = CODE;
    return keto::crypto::HashGenerator().generateHash(
        keto::crypto::SecureVectorUtils().copyStringToSecure(code));
}

keto::crypto::SecureVector getEncodedKey() {
    return keyPairCreator.getEncodedKey();
}

std::vector<uint8_t> getCode() {
    std::string code = CODE;
    keto::key_tools::ContentEncryptor contentEncryptor(keyPairCreator.getSecret(), 
            keyPairCreator.getEncodedKey(),
            keto::server_common::VectorUtils().copyStringToVector(code));
    return contentEncryptor.getEncryptedContent();
}

}


}
}



BOOST_AUTO_TEST_CASE( hash_generator_test ) {
    
    keto::software_consensus::test_module::getModuleSignature();
    keto::software_consensus::test_module::getModuleKey();
    
    keto::software_consensus::SourceVersionMap sourceVersionMap;
    sourceVersionMap["SourceClass1_header"] = &keto::software_consensus::SourceClass1::getHeaderVersion;
    sourceVersionMap["SourceClass1_source"] = &keto::software_consensus::SourceClass1::getSourceVersion;
    sourceVersionMap["SourceClass2_header"] = &keto::software_consensus::SourceClass2::getHeaderVersion;
    sourceVersionMap["SourceClass2_source"] = &keto::software_consensus::SourceClass2::getSourceVersion;
    
    
    keto::software_consensus::ConsensusHashScriptInfoVector consensusVector;
    
    consensusVector.push_back(std::make_shared<keto::software_consensus::ConsensusHashScriptInfo>(
            &keto::software_consensus::example_code1::getHash,
            &keto::software_consensus::example_code1::getEncodedKey,
            &keto::software_consensus::example_code1::getCode));
    consensusVector.push_back(std::make_shared<keto::software_consensus::ConsensusHashScriptInfo>(
            &keto::software_consensus::example_code2::getHash,
            &keto::software_consensus::example_code2::getEncodedKey,
            &keto::software_consensus::example_code2::getCode));
    
    
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator = 
            keto::software_consensus::ConsensusHashGenerator::initInstance(
            consensusVector,
            &keto::software_consensus::test_module::getModuleSignature,
            &keto::software_consensus::test_module::getModuleKey,
            sourceVersionMap);
    
    consensusHashGenerator->setSession(keto::software_consensus::example_code1::keyPairCreator.getSecret());
    
    keto::crypto::HashGenerator hashGenerator;
    
    keto::asn1::HashHelper previousHash(hashGenerator.generateHash("this is a test"));
    
    keto::crypto::SecureVector seed = consensusHashGenerator->generateSeed(previousHash);
    consensusHashGenerator->generateHash(seed);
    
}