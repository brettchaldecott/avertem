/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SessionKeyManager.cpp
 * Author: ubuntu
 * 
 * Created on February 17, 2018, 8:46 AM
 */

#include <botan/hash.h>
#include <botan/rsa.h>
#include <botan/rng.h>
#include <botan/p11_randomgenerator.h>
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>


#include "keto/keystore/SessionKeyManager.hpp"

#include "KeyStore.pb.h"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "include/keto/keystore/SessionKeyManager.hpp"

namespace keto {
namespace keystore {

std::string SessionKeyManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
SessionKeyManager::SessionKeyManager() : rng(new Botan::AutoSeeded_RNG) {
}

SessionKeyManager::~SessionKeyManager() {
}

keto::event::Event SessionKeyManager::requestKey(const keto::event::Event& event) {
    keto::proto::SessionKeyRequest request = keto::server_common::fromEvent<keto::proto::SessionKeyRequest>(event);
    std::vector<uint8_t> sessionHash = keto::server_common::VectorUtils().copyStringToVector(
            request.session_hash());
    
    std::lock_guard<std::mutex> guard(mutex);
    if (!this->sessionKeys.count(sessionHash)) {
        Botan::RSA_PrivateKey privateKey(*rng.get(), 2048);
        this->sessionKeys[sessionHash] = Botan::PKCS8::BER_encode( privateKey );
    }
    
    keto::proto::SessionKeyResponse response;
    response.set_session_hash(request.session_hash());
    response.set_session_key(keto::server_common::VectorUtils().copyVectorToString(
            keto::crypto::SecureVectorUtils().copyFromSecure(this->sessionKeys[sessionHash])));
    return keto::server_common::toEvent<keto::proto::SessionKeyResponse>(response);
}

keto::event::Event SessionKeyManager::removeKey(const keto::event::Event& event) {
    keto::proto::SessionKeyExpireRequest request = keto::server_common::fromEvent<keto::proto::SessionKeyExpireRequest>(event);
    std::vector<uint8_t> sessionHash = keto::server_common::VectorUtils().copyStringToVector(
            request.session_hash());
    std::lock_guard<std::mutex> guard(mutex);
    if (this->sessionKeys.count(sessionHash)) {
        this->sessionKeys.erase(sessionHash);
    }
    keto::proto::SessionKeyExpireResponse response;
    response.set_session_hash(request.session_hash());
    response.set_success(true);
    return keto::server_common::toEvent<keto::proto::SessionKeyExpireResponse>(response);
}

    
}
}
