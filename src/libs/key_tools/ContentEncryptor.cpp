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

#include "keto/key_tools/ContentEncryptor.hpp"
#include "keto/key_tools/Constants.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_tools {

std::string ContentEncryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
ContentEncryptor::ContentEncryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& content) {
    
    keto::crypto::SecureVector encryptionKeyBits;
    for (int index = 0; index < encodedKey.size(); index++) {
        encryptionKeyBits.push_back(encodedKey[index] ^ secret[index]);
    }
    
    Botan::DataSource_Memory memoryDatasource(encryptionKeyBits);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);
    
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    
    Botan::PK_Encryptor_EME enc(*privateKey,*rng.get(), keto::key_tools::Constants::ENCRYPTION_PADDING);
    
    this->encyptedContent = enc.encrypt(content,*rng.get());
}

ContentEncryptor::ContentEncryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& content) {
    
    keto::crypto::SecureVector encryptionKeyBits;
    for (int index = 0; index < encodedKey.size(); index++) {
        encryptionKeyBits.push_back(encodedKey[index] ^ secret[index]);
    }
    
    Botan::DataSource_Memory memoryDatasource(encryptionKeyBits);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);
    
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    
    Botan::PK_Encryptor_EME enc(*privateKey,*rng.get(), keto::key_tools::Constants::ENCRYPTION_PADDING);
    
    this->encyptedContent = enc.encrypt(content,*rng.get());
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


}
}