//
// Created by Brett Chaldecott on 2019/01/28.
//

#ifndef KETO_ENCRYPTOR_HPP
#define KETO_ENCRYPTOR_HPP

#include <string>
#include <vector>
#include <map>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_store_utils {

class Encryptor {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    virtual std::vector<uint8_t> encrypt(const keto::crypto::SecureVector& value) const = 0;
};


}
}

#endif //KETO_ENCRYPTOR_HPP
