//
// Created by Brett Chaldecott on 2019/01/31.
//


#include <botan/stream_cipher.h>

#include "keto/keystore/KeyStoreWrapIndexEncryptor.hpp"
#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/keystore/Exception.hpp"
#include "keto/crypto/CipherBuilder.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace keystore {

std::string KeyStoreWrapIndexEncryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreWrapIndexEncryptor::~KeyStoreWrapIndexEncryptor() {
}

std::vector<uint8_t> KeyStoreWrapIndexEncryptor::encrypt(const keto::crypto::SecureVector& value) const {
    size_t numKeys = this->keyStoreWrapIndexManager->getNumberOfKeys();
    std::cout << "The number of keys is : " << numKeys << std::endl;
    if (numKeys == 0) {
        BOOST_THROW_EXCEPTION(keto::keystore::KeyStoreWrapIndexContainsNoKeysConfigured());
    }
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,numKeys-1);
    distribution(stdGenerator);

    keto::crypto::SecureVector content = value;

    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        uint8_t baseIndex = distribution(stdGenerator);
        uint8_t pIndex = distribution(stdGenerator);
        keto::crypto::SecureVector indexes;
        indexes.push_back(baseIndex);
        indexes.push_back(pIndex);


        keto::crypto::CipherBuilder cipherBuilder(this->keyStoreWrapIndexManager->getKey(baseIndex)->getPrivateKey());
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));
        cipher->set_key(cipherBuilder.derive(32,this->keyStoreWrapIndexManager->getKey(pIndex)->getPrivateKey()));
        cipher->set_iv(NULL,0);
        cipher->encrypt(content);
        content.insert(content.begin(),indexes.begin(),indexes.end());
    }
    return keto::crypto::SecureVectorUtils().copyFromSecure(content);

}

KeyStoreWrapIndexEncryptor::KeyStoreWrapIndexEncryptor(KeyStoreWrapIndexManager* keyStoreWrapIndexManager) :
        keyStoreWrapIndexManager(keyStoreWrapIndexManager) {

}

}
}