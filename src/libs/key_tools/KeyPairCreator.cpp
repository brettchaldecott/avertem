/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   KeyPairCreator.cpp
 * Author: ubuntu
 * 
 * Created on June 21, 2018, 3:04 AM
 */

#include <iostream>
#include <memory>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/rsa.h>
#include <botan/auto_rng.h>

#include "keto/key_tools/KeyPairCreator.hpp"

namespace keto {
namespace key_tools {

KeyPairCreator::KeyPairCreator() {
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    std::shared_ptr<Botan::Private_Key> secretKey(
            new Botan::RSA_PrivateKey(*generator, 2056));
    this->secret = Botan::PKCS8::BER_encode(*secretKey);
    std::shared_ptr<Botan::Private_Key> encryptionKey(
            new Botan::RSA_PrivateKey(*generator, 2056));
    keto::crypto::SecureVector encryptionKeyBits = Botan::PKCS8::BER_encode(*encryptionKey);
    for (int index = 0; index < encryptionKeyBits.size(); index++) {
        this->encodedKey.push_back(encryptionKeyBits[index] ^ this->secret[index]);
    }
}

KeyPairCreator::~KeyPairCreator() {
}

keto::crypto::SecureVector KeyPairCreator::getSecret() {
    return this->secret;
}

keto::crypto::SecureVector KeyPairCreator::getEncodedKey() {
    return this->encodedKey;
}


}
}
