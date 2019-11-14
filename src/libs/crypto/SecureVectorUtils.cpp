/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SecureVectorUtils.cpp
 * Author: ubuntu
 * 
 * Created on February 16, 2018, 11:42 AM
 */

#include <sstream>
#include <iterator>

#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace crypto {

std::string SecureVectorUtils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
SecureVectorUtils::SecureVectorUtils() {
}

SecureVectorUtils::~SecureVectorUtils() {
}

std::vector<uint8_t> SecureVectorUtils::copyFromSecure(const SecureVector& secureVector) {
    SecureVector copySecureVector = secureVector;
    return copyFromSecure(copySecureVector);
}

std::vector<uint8_t> SecureVectorUtils::copyFromSecure(SecureVector& secureVector) {
    std::vector<uint8_t> result(secureVector.begin(),secureVector.end());
    return result;
}

SecureVector SecureVectorUtils::copyToSecure(const std::vector<uint8_t>& vector) {
    return SecureVector(vector.begin(),vector.end());
}

SecureVector SecureVectorUtils::copyToSecure(std::vector<uint8_t>& vector) {
    return SecureVector(vector.begin(),vector.end());
}


SecureVector SecureVectorUtils::copyStringToSecure(const std::string& str) {
    SecureVector result(str.begin(),str.end());
    return result;
}

std::string SecureVectorUtils::copySecureToString(const SecureVector& vec) {
    std::stringstream ss; 
    std::copy(vec.begin(), vec.end(),
         std::ostream_iterator<uint8_t>(ss));
    return ss.str();
}


SecureVector SecureVectorUtils::bitwiseXor(const SecureVector& lhs,const SecureVector& rhs) {
    SecureVector result;
    for (int index = 0; index < lhs.size() && index < rhs.size(); index++) {
        result.push_back(lhs[index] ^ rhs[index]);
    }
    return result;
}

}
}
