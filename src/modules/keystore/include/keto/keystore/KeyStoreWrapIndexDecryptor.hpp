//
// Created by Brett Chaldecott on 2019/01/31.
//

#ifndef KETO_KEYSTOREWRAPINDEXDECRYPTOR_HPP
#define KETO_KEYSTOREWRAPINDEXDECRYPTOR_HPP

#include <string>
#include <vector>
#include <map>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/key_store_utils/Encryptor.hpp"

namespace keto {
namespace keystore {

class KeyStoreWrapIndexManager;

class KeyStoreWrapIndexDecryptor;
typedef std::shared_ptr<KeyStoreWrapIndexDecryptor> KeyStoreWrapIndexDecryptorPtr;

class KeyStoreWrapIndexDecryptor : virtual public keto::key_store_utils::Encryptor {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    friend class KeyStoreWrapIndexManager;

    KeyStoreWrapIndexDecryptor(const KeyStoreWrapIndexDecryptor& keyStoreWrapIndexDecryptor) = delete;
    virtual ~KeyStoreWrapIndexDecryptor();

    keto::crypto::SecureVector decrypt(const std::vector<uint8_t>& value) const;

private:
    KeyStoreWrapIndexManager* keyStoreWrapIndexManager;

    KeyStoreWrapIndexDecryptor(KeyStoreWrapIndexManager* keyStoreWrapIndexManager);

};

}
}


#endif //KETO_KEYSTOREWRAPINDEXDECRYPTOR_HPP
