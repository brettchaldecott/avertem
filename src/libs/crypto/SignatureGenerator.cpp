/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignatureGenerator.cpp
 * Author: brett chaldecott
 * 
 * Created on February 6, 2018, 11:27 AM
 */

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/ecdsa.h>


#include "keto/crypto/SignatureGenerator.hpp"
#include "keto/crypto/Constants.hpp"
#include "include/keto/crypto/KeyLoader.hpp"

namespace keto {
namespace crypto {

std::string SignatureGenerator::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignatureGenerator::SignatureGenerator(const keto::crypto::SecureVector& key) : 
    key(key) {
}

SignatureGenerator::SignatureGenerator(const keto::crypto::KeyLoaderPtr& loader) : loader(loader) {
    
}

SignatureGenerator::~SignatureGenerator() {
}


std::vector<uint8_t> SignatureGenerator::sign(std::vector<uint8_t>& value) {

    std::shared_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    std::shared_ptr<Botan::Private_Key> privateKey = loadKey(rng);
    // present 
    Botan::PK_Signer signer(*privateKey, *rng, Constants::SIGNATURE_TYPE);
    return signer.sign_message(value, *rng);
}


std::vector<uint8_t> SignatureGenerator::sign(const keto::crypto::SecureVector& value) {
    std::shared_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    std::shared_ptr<Botan::Private_Key> privateKey = loadKey(rng);

    // present
    try {
        Botan::PK_Signer signer(*privateKey, *rng, Constants::SIGNATURE_TYPE);
        return signer.sign_message(value, *rng);
    } catch (...) {
        // fall back to older emsa signature
        try {
            Botan::PK_Signer signer(*privateKey, *rng, Constants::EMSA1_SIGNATURE_TYPE);
            return signer.sign_message(value, *rng);
        } catch (...) {
            KETO_LOG_DEBUG << "Failed to sign the message";
            throw;
        }
    }
}


std::shared_ptr<Botan::Private_Key> SignatureGenerator::loadKey(std::shared_ptr<Botan::RandomNumberGenerator> rng) {
    std::shared_ptr<Botan::Private_Key> privateKey;
    if (this->loader) {
        return this->loader->getPrivateKey();
    } else {
        if (key[0] == 0x0 || key[0] == 0x0) {
            KETO_LOG_DEBUG << "Load the key using secp information";
            Botan::EC_Group ecGroup("secp256k1");
            Botan::BigInt bigInt(key.data()+1,key.size()-1);
            return std::shared_ptr<Botan::Private_Key>(new Botan::ECDSA_PrivateKey(*rng,ecGroup,bigInt));
        } else {
            Botan::DataSource_Memory memoryDatasource(key);
            return
                    Botan::PKCS8::load_key(memoryDatasource);
        }

    }
}


}
}
