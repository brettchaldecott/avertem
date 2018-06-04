/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HashBuilder.cpp
 * Author: ubuntu
 * 
 * Created on June 1, 2018, 10:10 AM
 */

#include <sstream>

#include "keto/software_consensus/HashBuilder.hpp"
#include "keto/crypto/HashGenerator.hpp"

namespace keto {
namespace software_consensus {

HashBuilder::HashBuilder() {
}

HashBuilder::~HashBuilder() {
}

HashBuilder& HashBuilder::operator << (const std::string& value) {
    for (char character : value) {
        secureVector.push_back(character);
    }
    
    return *this;
}

HashBuilder& HashBuilder::operator << (const keto::crypto::SecureVector& value) {
    for (uint8_t character : value) {
        secureVector.push_back(character);
    }
    
    return *this;
}

HashBuilder& HashBuilder::operator << (char value) {
    secureVector.push_back(value);
    
    return *this;
}


HashBuilder& HashBuilder::operator << (long value) {
    std::stringstream ss;
    ss << value;
    
    for (uint8_t character : ss.str()) {
        secureVector.push_back(character);
    }
    return *this;
}

HashBuilder& HashBuilder::operator << (float value) {
    std::stringstream ss;
    ss << value;
    
    for (uint8_t character : ss.str()) {
        secureVector.push_back(character);
    }
    return *this;
}

keto::crypto::SecureVector HashBuilder::generateHash() {
    return keto::crypto::HashGenerator().generateHash(secureVector);
}

}
}
