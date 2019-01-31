//
// Created by Brett Chaldecott on 2019/01/31.
//

#ifndef KETO_KEYSTOREWRAPINDEXENCRYPTOR_HPP
#define KETO_KEYSTOREWRAPINDEXENCRYPTOR_HPP

#include <string>
#include <vector>
#include <map>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/key_store_utils/Encryptor.hpp"

namespace keto {
namespace keystore {

class KeyStoreWrapIndexManager;

class KeyStoreWrapIndexEncryptor;
typedef std::shared_ptr<KeyStoreWrapIndexEncryptor> KeyStoreWrapIndexEncryptorPtr;

class KeyStoreWrapIndexEncryptor : virtual public keto::key_store_utils::Encryptor {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();


    friend class KeyStoreWrapIndexManager;

    KeyStoreWrapIndexEncryptor(const KeyStoreWrapIndexEncryptor& keyStoreWrapIndexEncryptor) = delete;
    virtual ~KeyStoreWrapIndexEncryptor();

    std::vector<uint8_t> encrypt(const keto::crypto::SecureVector& value) const;

private:
    KeyStoreWrapIndexManager* keyStoreWrapIndexManager;

    KeyStoreWrapIndexEncryptor(KeyStoreWrapIndexManager* keyStoreWrapIndexManager);

};

}
}


#endif //KETO_KEYSTOREWRAPINDEXENCRYPTOR_HPP
