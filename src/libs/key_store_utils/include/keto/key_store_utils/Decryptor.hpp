//
// Created by Brett Chaldecott on 2019/01/28.
//

#ifndef KETO_DECRYPTOR_HPP
#define KETO_DECRYPTOR_HPP

#include <string>
#include <vector>
#include <map>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_store_utils {

class Decryptor {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    virtual keto::crypto::SecureVector decrypt(const std::vector<uint8_t>& value) const = 0;
};
}
}

#endif //KETO_DECRYPTOR_HPP
