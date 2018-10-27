/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HashGenerator.cpp
 * Author: ubuntu
 * 
 * Created on February 6, 2018, 2:32 PM
 */

#include <algorithm>

#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/Constants.hpp"

namespace keto {
namespace crypto {

std::string HashGenerator::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

HashGenerator::HashGenerator() : 
    hash256(Botan::HashFunction::create(Constants::HASH_TYPE)) {
}

HashGenerator::~HashGenerator() {
}

keto::crypto::SecureVector HashGenerator::generateHash(const keto::crypto::SecureVector& bytes) {
    return hash256->process(bytes);
}


keto::crypto::SecureVector HashGenerator::generateHash(const std::vector<uint8_t>& bytes) {
    return hash256->process(bytes);
}

keto::crypto::SecureVector HashGenerator::generateHash(const std::string& stringValue) {
    const std::vector<uint8_t> bytes(stringValue.begin(),stringValue.end());
    return hash256->process(bytes);
}

}
}