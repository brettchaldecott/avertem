//
// Created by Brett Chaldecott on 2019/01/31.
//


#include <botan/stream_cipher.h>

#include "keto/keystore/KeyStoreWrapIndexDecryptor.hpp"
#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/crypto/CipherBuilder.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


namespace keto {
namespace keystore {


std::string KeyStoreWrapIndexDecryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreWrapIndexDecryptor::~KeyStoreWrapIndexDecryptor() {

}

keto::crypto::SecureVector KeyStoreWrapIndexDecryptor::decrypt(const std::vector<uint8_t>& value) const {
    keto::crypto::SecureVector content = keto::crypto::SecureVectorUtils().copyToSecure(value);
    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        keto::crypto::SecureVector indexes(&content[0],&content[2]);
        keto::crypto::SecureVector encryptedValue(&content[2],&content[content.size()]);

        uint8_t baseIndex = indexes[0];
        uint8_t pIndex = indexes[1];

        keto::crypto::CipherBuilder cipherBuilder(this->keyStoreWrapIndexManager->getKey(baseIndex)->getPrivateKey());
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create("ChaCha(20)"));
        cipher->set_key(cipherBuilder.derive(32,this->keyStoreWrapIndexManager->getKey(pIndex)->getPrivateKey()));
        cipher->set_iv(NULL,0);
        cipher->decrypt(encryptedValue);

        content = encryptedValue;
    }
    return content;
}

KeyStoreWrapIndexDecryptor::KeyStoreWrapIndexDecryptor(KeyStoreWrapIndexManager* keyStoreWrapIndexManager) :
        keyStoreWrapIndexManager(keyStoreWrapIndexManager) {

}


}
}