//
// Created by Brett Chaldecott on 2019/04/04.
//

#include "keto/crypto/Secp256K1Utils.hpp"
#include "wally.hpp"
#include "keto/server_common/ModuleSessionManager.hpp"
#include <botan/ecdsa.h>
#include <botan/hex.h>

namespace keto {
namespace crypto {


std::string Secp256K1Utils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

Secp256K1Utils::Secp256K1Memory::Secp256K1Memory() {
}

Secp256K1Utils::Secp256K1Memory::~Secp256K1Memory() {
    Secp256K1Utils::Secp256K1UtilsScope::fin();
}

static Secp256K1Utils::Secp256K1UtilsScopePtr singleton;

Secp256K1Utils::Secp256K1UtilsScope::Secp256K1UtilsScope() {
    wally_init(0);
    keto::server_common::ModuleSessionManager::addSession(
            keto::server_common::ModuleSessionPtr(new Secp256K1Utils::Secp256K1Memory()));
}

Secp256K1Utils::Secp256K1UtilsScope::~Secp256K1UtilsScope() {
    wally_cleanup(0);
}

void Secp256K1Utils::Secp256K1UtilsScope::init() {
    if (!singleton) {
        singleton = Secp256K1Utils::Secp256K1UtilsScopePtr(new Secp256K1UtilsScope());
    }
}

void Secp256K1Utils::Secp256K1UtilsScope::fin() {
    singleton.reset();
}

bool Secp256K1Utils::verifySignature(const std::vector<uint8_t>& bits, std::vector<uint8_t> message,
                     const std::vector<uint8_t> &signature) {
    Secp256K1Utils::Secp256K1UtilsScope::init();
    //KETO_LOG_DEBUG << "The key is a ecdsa key";

    //KETO_LOG_DEBUG << "validate the signature : ";
    //KETO_LOG_DEBUG << "Key : " << Botan::hex_encode(bits);
    //KETO_LOG_DEBUG << "Message : " << Botan::hex_encode(message);
    //KETO_LOG_DEBUG << "Signature : " << Botan::hex_encode(signature);
    if (WALLY_OK ==
            wally_ec_sig_verify(bits.data(),EC_PUBLIC_KEY_LEN,message.data(),message.size(),EC_FLAG_ECDSA,signature.data(),signature.size())) {
        //KETO_LOG_DEBUG << "The signature is valid";
        return true;
    }
    //KETO_LOG_DEBUG << "The signature is invalid";
    return false;
}


}
}
