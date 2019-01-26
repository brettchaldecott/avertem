//
// Created by Brett Chaldecott on 2019/01/25.
//

#ifndef KETO_KEYUTILS_HPP
#define KETO_KEYUTILS_HPP

#include <vector>
#include <string>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/ecies.h>
#include <botan/filter.h>
#include <botan/filters.h>
#include <botan/dlies.h>
#include <botan/hex.h>


#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_tools {

class KeyUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    KeyUtils();
    KeyUtils(const KeyUtils& orig) = default;
    virtual ~KeyUtils();

    keto::crypto::SecureVector getDerivedKey(
            const keto::crypto::SecureVector& secret,
            const keto::crypto::SecureVector& encodedKey);

    Botan::SymmetricKey generateCipher(
            const keto::crypto::SecureVector& secret,
            const keto::crypto::SecureVector& derived);
};


}
}



#endif //KETO_KEYUTILS_HPP
