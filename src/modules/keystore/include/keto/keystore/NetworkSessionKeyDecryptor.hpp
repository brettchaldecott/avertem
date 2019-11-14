//
// Created by Brett Chaldecott on 2019/01/24.
//

#ifndef KETO_NETWORKSESSIONKEYDECRYPTOR_HPP
#define KETO_NETWORKSESSIONKEYDECRYPTOR_HPP

#include <string>
#include <vector>
#include <map>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/key_store_utils/Decryptor.hpp"

namespace keto {
namespace keystore {

class NetworkSessionKeyManager;

class NetworkSessionKeyDecryptor;
typedef std::shared_ptr<NetworkSessionKeyDecryptor> NetworkSessionKeyDecryptorPtr;

class NetworkSessionKeyDecryptor : virtual public keto::key_store_utils::Decryptor {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    friend class NetworkSessionKeyManager;

    NetworkSessionKeyDecryptor(const NetworkSessionKeyDecryptor& orig) = delete;
    virtual ~NetworkSessionKeyDecryptor();

    keto::crypto::SecureVector decrypt(const std::vector<uint8_t>& value) const;

private:
    NetworkSessionKeyDecryptor();

};


}
}


#endif //KETO_NETWORKSESSIONKEYDECRYPTOR_HPP
