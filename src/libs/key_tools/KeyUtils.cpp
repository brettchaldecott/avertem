//
// Created by Brett Chaldecott on 2019/01/25.
//

#include "keto/key_tools/KeyUtils.hpp"

#include "keto/crypto/CipherBuilder.hpp"

namespace keto {
namespace key_tools {

std::string KeyUtils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyUtils::KeyUtils() {

}

KeyUtils::~KeyUtils() {

}

keto::crypto::SecureVector KeyUtils::getDerivedKey(
        const keto::crypto::SecureVector& secret,
        const keto::crypto::SecureVector& encodedKey) {
    keto::crypto::SecureVector encryptionKeyBits;
    for (int index = 0; index < encodedKey.size(); index++) {
        encryptionKeyBits.push_back(encodedKey[index] ^ secret[index]);
    }
    return encryptionKeyBits;
}

Botan::SymmetricKey KeyUtils::generateCipher(
        const keto::crypto::SecureVector& secret,
        const keto::crypto::SecureVector& derived) {

    Botan::DataSource_Memory secretDatasource(secret);
    std::shared_ptr<Botan::Private_Key> secretKey =
            Botan::PKCS8::load_key(secretDatasource);

    Botan::DataSource_Memory derivedDatasource(derived);
    std::shared_ptr<Botan::Private_Key> derivedKey =
            Botan::PKCS8::load_key(derivedDatasource);

    keto::crypto::CipherBuilder cipherBuilder(secretKey);
    return cipherBuilder.derive(32,derivedKey);
}



}
}