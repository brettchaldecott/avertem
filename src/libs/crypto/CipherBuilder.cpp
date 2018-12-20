//
// Created by Brett Chaldecott on 2018/12/15.
//

#include <math.h>

#include "keto/crypto/CipherBuilder.hpp"

#include "botan/dh.h"
#include "botan/dsa.h"
#include "botan/rsa.h"
#include "botan/elgamal.h"

#include "keto/crypto/Constants.hpp"
#include "keto/crypto/Exception.hpp"

namespace keto {
namespace crypto {

CipherBuilder::CipherBuilder(const PrivateKeyPtr& privateKeyPtr) : grp("modp/ietf/2048"){
    this->rng = std::shared_ptr<Botan::AutoSeeded_RNG>(new Botan::AutoSeeded_RNG());
    if (privateKeyPtr->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PrivateKey> rsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PrivateKey>(privateKeyPtr);
        this->privateKeyPtr = std::shared_ptr<Botan::Private_Key>(new Botan::DH_PrivateKey(*rng, grp,rsaPrivateKeyPtr->get_d()));
    } else if (privateKeyPtr->algo_name() == "DSA" || privateKeyPtr->algo_name() == "DH") {
        this->privateKeyPtr = privateKeyPtr;
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat());
    }
}


CipherBuilder::~CipherBuilder() {

}


Botan::SymmetricKey CipherBuilder::derive(const PrivateKeyPtr& privateKeyPtr) {
    return derive(2048, privateKeyPtr);
}


Botan::SymmetricKey CipherBuilder::derive(size_t size, const PrivateKeyPtr& privateKeyPtr) {
    std::shared_ptr<Botan::PK_Key_Agreement> ka(new Botan::PK_Key_Agreement(*this->privateKeyPtr,*rng,"Raw"));
    Botan::SymmetricKey sementicKey;
    if (privateKeyPtr->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PrivateKey> rsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(rsaPrivateKeyPtr->get_p()));
    } else if (privateKeyPtr->algo_name() == "DSA") {
        std::shared_ptr<Botan::DSA_PrivateKey> dsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::DSA_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(dsaPrivateKeyPtr->get_x()));
    } else if (privateKeyPtr->algo_name() == "DH") {
        std::shared_ptr<Botan::DH_PrivateKey> dhPrivateKeyPtr = std::dynamic_pointer_cast<Botan::DH_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(dhPrivateKeyPtr->get_x()));
    } else if (privateKeyPtr->algo_name() == "ElGamal") {
        std::shared_ptr<Botan::ElGamal_PrivateKey> elPrivateKeyPtr = std::dynamic_pointer_cast<Botan::ElGamal_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(elPrivateKeyPtr->get_x()));
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat(privateKeyPtr->algo_name()));
    }
    return resizeCipher(size, sementicKey);
}

std::shared_ptr<Botan::DH_PrivateKey> CipherBuilder::deriveKey(const PrivateKeyPtr& privateKeyPtr) {
    std::shared_ptr<Botan::PK_Key_Agreement> ka(new Botan::PK_Key_Agreement(*this->privateKeyPtr,*rng,"Raw"));
    Botan::SymmetricKey sementicKey;
    if (privateKeyPtr->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PrivateKey> rsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(rsaPrivateKeyPtr->get_p()));
    } else if (privateKeyPtr->algo_name() == "DSA") {
        std::shared_ptr<Botan::DSA_PrivateKey> dsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::DSA_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(dsaPrivateKeyPtr->get_x()));
    } else if (privateKeyPtr->algo_name() == "DH") {
        std::shared_ptr<Botan::DH_PrivateKey> dhPrivateKeyPtr = std::dynamic_pointer_cast<Botan::DH_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(dhPrivateKeyPtr->get_x()));
    } else if (privateKeyPtr->algo_name() == "ElGamal") {
        std::shared_ptr<Botan::ElGamal_PrivateKey> elPrivateKeyPtr = std::dynamic_pointer_cast<Botan::ElGamal_PrivateKey>(privateKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(elPrivateKeyPtr->get_x()));
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat(privateKeyPtr->algo_name()));
    }
    Botan::DL_Group grp("modp/ietf/2048");
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    std::cout << "The p size is : " << grp.get_p().size() << std::endl;
    return std::shared_ptr<Botan::DH_PrivateKey>(new Botan::DH_PrivateKey(*generator, grp,
                                                                                    Botan::BigInt(sementicKey.bits_of())));

}

Botan::SymmetricKey CipherBuilder::derive(const PublicKeyPtr& publicKeyPtr) {
    return derive(2048,publicKeyPtr);
}

Botan::SymmetricKey CipherBuilder::derive(size_t size, const PublicKeyPtr& publicKeyPtr) {
    std::shared_ptr<Botan::PK_Key_Agreement> ka(new Botan::PK_Key_Agreement(*this->privateKeyPtr,*rng,"Raw"));
    Botan::SymmetricKey sementicKey;
    if (publicKeyPtr->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PublicKey> rsaPublicKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(rsaPublicKeyPtr->get_n()));
    } else if (publicKeyPtr->algo_name() == "DSA") {
        std::shared_ptr<Botan::DSA_PublicKey> dsaPublicKeyPtr = std::dynamic_pointer_cast<Botan::DSA_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(dsaPublicKeyPtr->get_y()));
    } else if (publicKeyPtr->algo_name() == "DH") {
        std::shared_ptr<Botan::DH_PublicKey> dhPublicKeyPtr = std::dynamic_pointer_cast<Botan::DH_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(dhPublicKeyPtr->get_y()));
    } else if (publicKeyPtr->algo_name() == "ElGamal") {
        std::shared_ptr<Botan::ElGamal_PublicKey> elPrivateKeyPtr = std::dynamic_pointer_cast<Botan::ElGamal_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(size,Botan::BigInt::encode(elPrivateKeyPtr->get_y()));
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat());
    }

    return resizeCipher(size, sementicKey);
}


std::shared_ptr<Botan::DH_PrivateKey> CipherBuilder::deriveKey(const PublicKeyPtr& publicKeyPtr) {
    std::shared_ptr<Botan::PK_Key_Agreement> ka(new Botan::PK_Key_Agreement(*this->privateKeyPtr,*rng,"Raw"));
    Botan::SymmetricKey sementicKey;
    if (publicKeyPtr->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PublicKey> rsaPublicKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(rsaPublicKeyPtr->get_n()));
    } else if (publicKeyPtr->algo_name() == "DSA") {
        std::shared_ptr<Botan::DSA_PublicKey> dsaPublicKeyPtr = std::dynamic_pointer_cast<Botan::DSA_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(dsaPublicKeyPtr->get_y()));
    } else if (publicKeyPtr->algo_name() == "DH") {
        std::shared_ptr<Botan::DH_PublicKey> dhPublicKeyPtr = std::dynamic_pointer_cast<Botan::DH_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(dhPublicKeyPtr->get_y()));
    } else if (publicKeyPtr->algo_name() == "ElGamal") {
        std::shared_ptr<Botan::ElGamal_PublicKey> elPrivateKeyPtr = std::dynamic_pointer_cast<Botan::ElGamal_PublicKey>(publicKeyPtr);
        sementicKey = ka->derive_key(ka->agreed_value_size(),Botan::BigInt::encode(elPrivateKeyPtr->get_y()));
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat());
    }
    Botan::DL_Group grp("modp/ietf/2048");
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    return std::shared_ptr<Botan::DH_PrivateKey>(new Botan::DH_PrivateKey(*generator, grp,
                                                                      Botan::BigInt(sementicKey.bits_of())));
}


Botan::SymmetricKey CipherBuilder::resizeCipher(size_t size, Botan::SymmetricKey key) {
    Botan::BigInt modulas = Botan::BigInt::power_of_2(size*8);
    Botan::BigInt bigInt(key.begin(),key.size());
    bigInt%=modulas;

    return Botan::SymmetricKey(Botan::BigInt::encode(bigInt));
}

}
}

