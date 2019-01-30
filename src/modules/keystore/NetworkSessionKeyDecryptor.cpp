//
// Created by Brett Chaldecott on 2019/01/24.
//

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/stream_cipher.h>

#include "keto/keystore/NetworkSessionKeyDecryptor.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/crypto/CipherBuilder.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/keystore/Constants.hpp"

namespace keto {
namespace keystore {

std::string NetworkSessionKeyDecryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

NetworkSessionKeyDecryptor::~NetworkSessionKeyDecryptor() {

}

keto::crypto::SecureVector NetworkSessionKeyDecryptor::decrypt(const std::vector<uint8_t>& value) const {
    keto::crypto::SecureVector content = keto::crypto::SecureVectorUtils().copyToSecure(value);
    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        keto::crypto::SecureVector indexes(&content[0],&content[2]);
        keto::crypto::SecureVector encryptedValue(&content[2],&content[content.size()]);

        uint8_t baseIndex = indexes[0];
        uint8_t pIndex = indexes[1];

        keto::crypto::CipherBuilder cipherBuilder(this->networkSessionKeyManager->getKey(baseIndex)->getPrivateKey());
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create("ChaCha(20)"));
        cipher->set_key(cipherBuilder.derive(32,this->networkSessionKeyManager->getKey(pIndex)->getPrivateKey()));
        cipher->set_iv(NULL,0);
        cipher->decrypt(encryptedValue);

        content = encryptedValue;
    }
    return content;
}

NetworkSessionKeyDecryptor::NetworkSessionKeyDecryptor(NetworkSessionKeyManager* networkSessionKeyManager) :
    networkSessionKeyManager(networkSessionKeyManager) {

}


}
}