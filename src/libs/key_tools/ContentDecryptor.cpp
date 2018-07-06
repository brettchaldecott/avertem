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

#include "keto/key_tools/ContentDecryptor.hpp"
#include "keto/key_tools/Constants.hpp"

namespace keto {
namespace key_tools {


ContentDecryptor::ContentDecryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& encyptedContent) {
    
    keto::crypto::SecureVector encryptionKeyBits = getDerivedKey(secret,encodedKey);
    
    Botan::DataSource_Memory memoryDatasource(encryptionKeyBits);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);
    
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    Botan::PK_Decryptor_EME dec(*privateKey,*rng.get(), keto::key_tools::Constants::ENCRYPTION_PADDING);
    this->content = dec.decrypt(encyptedContent);
}

ContentDecryptor::ContentDecryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& encyptedContent) {
    
    keto::crypto::SecureVector encryptionKeyBits = getDerivedKey(secret,encodedKey);
    
    Botan::DataSource_Memory memoryDatasource(encryptionKeyBits);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);
    
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    Botan::PK_Decryptor_EME dec(*privateKey,*rng.get(), keto::key_tools::Constants::ENCRYPTION_PADDING);
    this->content = dec.decrypt(encyptedContent);
}

ContentDecryptor::~ContentDecryptor() {
}

keto::crypto::SecureVector ContentDecryptor::getDerivedKey(const keto::crypto::SecureVector& secret,
        const keto::crypto::SecureVector& encodedKey) {
    keto::crypto::SecureVector encryptionKeyBits;
    for (int index = 0; index < encodedKey.size(); index++) {
        encryptionKeyBits.push_back(encodedKey[index] ^ secret[index]);
    }
    return encryptionKeyBits;
}

keto::crypto::SecureVector ContentDecryptor::getContent() {
    return this->content;
}

ContentDecryptor::operator keto::crypto::SecureVector() {
    return this->content;
}


}
}