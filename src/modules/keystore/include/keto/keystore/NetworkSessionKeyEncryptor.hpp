//
// Created by Brett Chaldecott on 2019/01/24.
//

#ifndef KETO_NETWORKSESSIONENCRYPTOR_HPP
#define KETO_NETWORKSESSIONENCRYPTOR_HPP


#include <string>
#include <vector>
#include <map>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/key_store_utils/Encryptor.hpp"


namespace keto {
namespace keystore {

class NetworkSessionKeyManager;

class NetworkSessionKeyEncryptor;
typedef std::shared_ptr<NetworkSessionKeyEncryptor> NetworkSessionKeyEncryptorPtr;

class NetworkSessionKeyEncryptor : virtual public keto::key_store_utils::Encryptor {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    friend class NetworkSessionKeyManager;

    NetworkSessionKeyEncryptor(const NetworkSessionKeyEncryptor& orig) = delete;
    ~NetworkSessionKeyEncryptor();

    std::vector<uint8_t> encrypt(const keto::crypto::SecureVector& value) const;

private:
    NetworkSessionKeyManager* networkSessionKeyManager;

    NetworkSessionKeyEncryptor(NetworkSessionKeyManager* networkSessionKeyManager);
};


}
}



#endif //KETO_NETWORKSESSIONENCRYPTOR_HPP
