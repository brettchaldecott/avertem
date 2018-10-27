/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ContentSigner.cpp
 * Author: ubuntu
 * 
 * Created on July 5, 2018, 6:21 PM
 */

#include "keto/crypto/SignatureGenerator.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/key_tools/ContentSigner.hpp"

namespace keto {
namespace key_tools {
    
std::string ContentSigner::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ContentSigner::ContentSigner(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& content) {
    keto::crypto::SecureVector encryptionKeyBits;
    for (int index = 0; index < encodedKey.size(); index++) {
        encryptionKeyBits.push_back(encodedKey[index] ^ secret[index]);
    }
    std::vector<uint8_t> _content = content;
    this->signature = keto::crypto::SecureVectorUtils().copyToSecure(
            keto::crypto::SignatureGenerator(encryptionKeyBits).sign(_content));
}

ContentSigner::ContentSigner(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& content) {
    keto::crypto::SecureVector encryptionKeyBits;
    for (int index = 0; index < encodedKey.size(); index++) {
        encryptionKeyBits.push_back(encodedKey[index] ^ secret[index]);
    }
    
    this->signature = keto::crypto::SecureVectorUtils().copyToSecure(
            keto::crypto::SignatureGenerator(encryptionKeyBits).sign(content));
}

ContentSigner::~ContentSigner() {
}

keto::crypto::SecureVector ContentSigner::getSignature() {
    return this->signature;
}

}
}