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

#include "keto/keystore/NetworkSessionKeyEncryptor.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/crypto/CipherBuilder.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/keystore/Constants.hpp"

namespace keto {
namespace keystore {

std::string NetworkSessionKeyEncryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

NetworkSessionKeyEncryptor::~NetworkSessionKeyEncryptor() {

}

std::vector<uint8_t> NetworkSessionKeyEncryptor::encrypt(const keto::crypto::SecureVector& value) const {
    size_t numKeys = this->networkSessionKeyManager->getNumberOfKeys();

    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,numKeys-1);

    keto::crypto::SecureVector content = value;

    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        uint8_t baseIndex = distribution(stdGenerator);
        uint8_t pIndex = distribution(stdGenerator);
        keto::crypto::SecureVector indexes;
        indexes.push_back(baseIndex);
        indexes.push_back(pIndex);


        keto::crypto::CipherBuilder cipherBuilder(this->networkSessionKeyManager->getKey(baseIndex)->getPrivateKey());
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));
        cipher->set_key(cipherBuilder.derive(32,this->networkSessionKeyManager->getKey(pIndex)->getPrivateKey()));
        cipher->set_iv(NULL,0);
        cipher->encrypt(content);
        content.insert(content.begin(),indexes.begin(),indexes.end());
    }
    return keto::crypto::SecureVectorUtils().copyFromSecure(content);
}

NetworkSessionKeyEncryptor::NetworkSessionKeyEncryptor(NetworkSessionKeyManager* networkSessionKeyManager) : networkSessionKeyManager(networkSessionKeyManager) {
}

}
}