//
// Created by Brett Chaldecott on 2018/12/03.
//

#ifndef KETO_KEYBUILDER_HPP
#define KETO_KEYBUILDER_HPP

#include <string>
#include <memory>

#include <boost/filesystem/path.hpp>

#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/bigint.h>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace crypto {

class KeyBuilder;
typedef std::shared_ptr<KeyBuilder> KeyBuilderPtr;
typedef std::vector<Botan::BigInt> BigIntVector;

class KeyBuilder {
public:
    KeyBuilder();
    KeyBuilder(const KeyBuilder& orig) = default;
    ~KeyBuilder();

    KeyBuilder& addBytes(
            const std::vector<uint8_t>& bytes);
    KeyBuilder& addPrivateKey(
            const std::shared_ptr<Botan::Private_Key>& bytes);
    KeyBuilder& addPublicKey(
            const std::shared_ptr<Botan::Public_Key>& bytes);


    std::shared_ptr<Botan::Private_Key> getPrivateKey();
    std::shared_ptr<Botan::Public_Key> getPublicKey();

private:
    std::shared_ptr<Botan::Private_Key> privateKeyPtr;
    BigIntVector privateKeys;
};


}
}


#endif //KETO_KEYBUILDER_HPP
