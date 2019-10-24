//
// Created by Brett Chaldecott on 2018/12/15.
//

#ifndef KETO_CIPHERBUILDER_HPP
#define KETO_CIPHERBUILDER_HPP

#ifndef BOTAN_NO_DEPRECATED_WARNINGS
#define BOTAN_NO_DEPRECATED_WARNINGS
#endif


#include <memory>
#include <string>

#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/dh.h>
#include <botan/elgamal.h>


#include "keto/crypto/Containers.hpp"

namespace keto {
namespace crypto {

typedef std::shared_ptr<Botan::Public_Key> PublicKeyPtr;
typedef std::shared_ptr<Botan::Private_Key> PrivateKeyPtr;
class CipherBuilder;
typedef std::shared_ptr<CipherBuilder> CipherBuilderPtr;

class CipherBuilder {
public:
    CipherBuilder(const PrivateKeyPtr& privateKeyPtr);
    CipherBuilder(const CipherBuilder& orig) = default;
    virtual ~CipherBuilder();


    Botan::SymmetricKey derive(const PrivateKeyPtr& privateKeyPtr);
    Botan::SymmetricKey derive(size_t size, const PrivateKeyPtr& privateKeyPtr);
    std::shared_ptr<Botan::DH_PrivateKey> deriveKey(const PrivateKeyPtr& privateKeyPtr);
    Botan::SymmetricKey derive(const PublicKeyPtr& publicKeyPtr);
    Botan::SymmetricKey derive(size_t size, const PublicKeyPtr& publicKeyPtr);
    std::shared_ptr<Botan::DH_PrivateKey> deriveKey(const PublicKeyPtr& privateKeyPtr);


private:
    Botan::DL_Group grp;
    PrivateKeyPtr privateKeyPtr;
    std::shared_ptr<Botan::RandomNumberGenerator> rng;
    std::shared_ptr<Botan::PK_Key_Agreement> pka;


    Botan::SymmetricKey resizeCipher(size_t size, Botan::SymmetricKey key);
};

}
}


#endif //KETO_CIPHERBUILDER_HPP
