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
#include "keto/keystore/Exception.hpp"

namespace keto {
namespace keystore {

std::string NetworkSessionKeyDecryptor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

NetworkSessionKeyDecryptor::~NetworkSessionKeyDecryptor() {

}

keto::crypto::SecureVector NetworkSessionKeyDecryptor::decrypt(const std::vector<uint8_t>& value) const {
    if (value.empty() || value .size() <= 4) {
        BOOST_THROW_EXCEPTION(EmptyDataToDecryptException());
    }
    uint8_t slot = value[0];
    keto::crypto::SecureVector content(&value[1],&value[value.size()]);
    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        //auto start = std::chrono::steady_clock::now();

        if (content.size() <= 3) {
            BOOST_THROW_EXCEPTION(CorruptedEncryptedValueException());
        }

        keto::crypto::SecureVector encryptedValue(&content[2],&content[content.size()]);

        uint8_t baseIndex = content[0];
        uint8_t pIndex = content[1];

        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr memoryVaultSessionKeyWrapperPtr =
                NetworkSessionKeyManager::getInstance()->getKey(slot,baseIndex);
        if (!memoryVaultSessionKeyWrapperPtr) {
            BOOST_THROW_EXCEPTION(NetworkSessionKeyNotFoundException());
        }
        keto::crypto::CipherBuilder cipherBuilder(memoryVaultSessionKeyWrapperPtr->getPrivateKey());
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr pIndexKeyWrapperPtr = NetworkSessionKeyManager::getInstance()->getKey(slot,pIndex);
        if (!pIndexKeyWrapperPtr) {
            BOOST_THROW_EXCEPTION(NetworkSessionKeyNotFoundException());
        }
        cipher->set_key(cipherBuilder.derive(32,pIndexKeyWrapperPtr->getPrivateKey()));
        cipher->set_iv(NULL,0);
        cipher->decrypt(encryptedValue);
        content = encryptedValue;
    }
    return content;
}

NetworkSessionKeyDecryptor::NetworkSessionKeyDecryptor() {

}


}
}
