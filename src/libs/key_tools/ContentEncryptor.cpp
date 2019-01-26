/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ContentEncryptor.cpp
 * Author: ubuntu
 * 
 * Created on June 21, 2018, 3:02 AM
 */

#include <iostream>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/stream_cipher.h>

#include "keto/key_tools/ContentEncryptor.hpp"
#include "keto/key_tools/Constants.hpp"
#include "keto/key_tools/KeyUtils.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_tools {

std::string ContentEncryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
ContentEncryptor::ContentEncryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& content) {
    keto::crypto::SecureVector encyptedContent = keto::crypto::SecureVectorUtils().copyToSecure(content);
    encrypt(secret, KeyUtils().getDerivedKey(secret,encodedKey),encyptedContent);
    this->encyptedContent = keto::crypto::SecureVectorUtils().copyFromSecure(encyptedContent);
}

ContentEncryptor::ContentEncryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& content) {
    keto::crypto::SecureVector encyptedContent = content;
    encrypt(secret, KeyUtils().getDerivedKey(secret,encodedKey),encyptedContent);
    this->encyptedContent = keto::crypto::SecureVectorUtils().copyFromSecure(encyptedContent);
}

ContentEncryptor::~ContentEncryptor() {
}

std::vector<uint8_t> ContentEncryptor::getEncryptedContent() {
    return this->encyptedContent;
}

keto::crypto::SecureVector ContentEncryptor::getEncryptedContent_locked() {
    return keto::crypto::SecureVectorUtils().copyToSecure(
            this->encyptedContent);
}

ContentEncryptor::operator std::vector<uint8_t>() {
    return this->encyptedContent;
}

void ContentEncryptor::encrypt(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& derived,
                               keto::crypto::SecureVector& content) {
    std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create("ChaCha(20)"));
    cipher->set_key(keto::key_tools::KeyUtils().generateCipher(secret,derived));
    cipher->set_iv(NULL,0);
    cipher->encrypt(content);

}

}
}