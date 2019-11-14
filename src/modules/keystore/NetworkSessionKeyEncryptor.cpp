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
    size_t numKeys = NetworkSessionKeyManager::getInstance()->getNumberOfKeys();

    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,numKeys-1);
    // pre init the value
    distribution(stdGenerator);

    keto::crypto::SecureVector content = value;

    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        //auto start = std::chrono::steady_clock::now();

        uint8_t baseIndex = distribution(stdGenerator);
        uint8_t pIndex = distribution(stdGenerator);
        keto::crypto::SecureVector indexes;
        indexes.push_back(baseIndex);
        indexes.push_back(pIndex);

        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::encrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] get base index [" << (int)baseIndex << "]";
        keto::crypto::CipherBuilder cipherBuilder(NetworkSessionKeyManager::getInstance()->getKey(baseIndex)->getPrivateKey());
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::encrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] create the cipher stream";
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::encrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] pIndex [" << (int)pIndex << "]";
        cipher->set_key(cipherBuilder.derive(32,NetworkSessionKeyManager::getInstance()->getKey(pIndex)->getPrivateKey()));
        cipher->set_iv(NULL,0);
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::encrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] before encryption";
        cipher->encrypt(content);
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::encrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] after encryption";
        content.insert(content.begin(),indexes.begin(),indexes.end());
    }
    //KETO_LOG_DEBUG << "The encypted content is : " << content.size();
    return keto::crypto::SecureVectorUtils().copyFromSecure(content);
}

NetworkSessionKeyEncryptor::NetworkSessionKeyEncryptor() {
}

}
}
