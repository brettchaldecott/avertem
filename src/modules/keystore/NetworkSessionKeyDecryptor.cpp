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
    keto::crypto::SecureVector content = keto::crypto::SecureVectorUtils().copyToSecure(value);
    if (content.empty()) {
        BOOST_THROW_EXCEPTION(EmptyDataToDecryptException());
    }
    //KETO_LOG_DEBUG << "The content decrypt : " << content.size();
    for (int level = 0; level < Constants::ONION_LEVELS; level++) {
        //auto start = std::chrono::steady_clock::now();

        keto::crypto::SecureVector indexes(&content[0],&content[2]);
        keto::crypto::SecureVector encryptedValue(&content[2],&content[content.size()]);

        uint8_t baseIndex = indexes[0];
        uint8_t pIndex = indexes[1];

        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::decrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "get base index [" << (int)baseIndex;
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr memoryVaultSessionKeyWrapperPtr =
                this->networkSessionKeyManager->getKey(baseIndex);
        if (!memoryVaultSessionKeyWrapperPtr) {
            BOOST_THROW_EXCEPTION(NetworkSessionKeyNotFoundException());
        }
        keto::crypto::CipherBuilder cipherBuilder(memoryVaultSessionKeyWrapperPtr->getPrivateKey());
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::decrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] pIndex [" << (int)pIndex << "]";
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr pIndexKeyWrapperPtr = this->networkSessionKeyManager->getKey(pIndex);
        if (!pIndexKeyWrapperPtr) {
            BOOST_THROW_EXCEPTION(NetworkSessionKeyNotFoundException());
        }
        cipher->set_key(cipherBuilder.derive(32,pIndexKeyWrapperPtr->getPrivateKey()));
        cipher->set_iv(NULL,0);
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::decrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] before decryption";
        cipher->decrypt(encryptedValue);
        //KETO_LOG_DEBUG << "[NetworkSessionKeyDecryptor::decrypt][" <<
        //    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] after decryption";
        content = encryptedValue;
    }
    return content;
}

NetworkSessionKeyDecryptor::NetworkSessionKeyDecryptor(NetworkSessionKeyManager* networkSessionKeyManager) :
    networkSessionKeyManager(networkSessionKeyManager) {

}


}
}
