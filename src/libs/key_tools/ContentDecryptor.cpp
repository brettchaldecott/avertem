/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ContentDecryptor.cpp
 * Author: ubuntu
 * 
 * Created on June 19, 2018, 11:00 AM
 */

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <keto/crypto/SecureVectorUtils.hpp>

#include "keto/key_tools/ContentDecryptor.hpp"
#include "keto/key_tools/Constants.hpp"
#include "keto/key_tools/KeyUtils.hpp"

namespace keto {
namespace key_tools {

std::string ContentDecryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ContentDecryptor::ContentDecryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& encyptedContent) {
    this->content = keto::crypto::SecureVectorUtils().copyToSecure(encyptedContent);
    decrypt(secret, KeyUtils().getDerivedKey(secret,encodedKey),this->content);
}

ContentDecryptor::ContentDecryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& encyptedContent) {
    this->content = encyptedContent;
    decrypt(secret, KeyUtils().getDerivedKey(secret,encodedKey),this->content);
}

ContentDecryptor::~ContentDecryptor() {
}

keto::crypto::SecureVector ContentDecryptor::getContent() {
    return this->content;
}

ContentDecryptor::operator keto::crypto::SecureVector() {
    return this->content;
}


void ContentDecryptor::decrypt(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& derived,
             keto::crypto::SecureVector& encyptedContent) {
    std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create("ChaCha(20)"));
    cipher->set_key(keto::key_tools::KeyUtils().generateCipher(secret,derived));
    cipher->set_iv(NULL,0);
    cipher->decrypt(encyptedContent);

}

}
}