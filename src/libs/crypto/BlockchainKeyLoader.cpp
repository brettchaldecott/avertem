//
// Created by Brett Chaldecott on 2019/03/27.
//

#include "keto/crypto/BlockchainPublicKeyLoader.hpp"

#include <botan/hex.h>
#include <botan/base64.h>
#include <botan/x509_key.h>
#include <botan/ecdsa.h>

namespace keto {
namespace crypto {


std::string BlockchainPublicKeyLoader::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockchainPublicKeyLoader::BlockchainPublicKeyLoader(const std::string& hexString) {
    std::vector<uint8_t> bytes = Botan::hex_decode(hexString,true);
    if (bytes[0] == 0x3 || bytes[0] == 0x2) {
        Botan::EC_Group ecGroup("secp256k1");
        this->publicKeyPtr = std::shared_ptr<Botan::Public_Key>(new Botan::ECDSA_PublicKey(ecGroup,ecGroup.OS2ECP(bytes)));
    } else {
        this->publicKeyPtr = std::shared_ptr<Botan::Public_Key>(Botan::X509::load_key(bytes));
    }
}

BlockchainPublicKeyLoader::BlockchainPublicKeyLoader(const std::vector<uint8_t>& bytes) {
    if (bytes[0] == 0x3 || bytes[0] == 0x2) {
        Botan::EC_Group ecGroup("secp256k1");
        this->publicKeyPtr = std::shared_ptr<Botan::Public_Key>(new Botan::ECDSA_PublicKey(ecGroup,ecGroup.OS2ECP(bytes)));
    } else {
        this->publicKeyPtr = std::shared_ptr<Botan::Public_Key>(Botan::X509::load_key(bytes));
    }
}

BlockchainPublicKeyLoader::~BlockchainPublicKeyLoader() {

}


std::shared_ptr<Botan::Public_Key> BlockchainPublicKeyLoader::getPublicKey() {
    return this->publicKeyPtr;
}

}
}