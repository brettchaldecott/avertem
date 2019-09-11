//
// Created by Brett Chaldecott on 2018/12/03.
//

#include "keto/crypto/KeyBuilder.hpp"
#include "keto/crypto/Exception.hpp"

#include "botan/dh.h"
#include "botan/dsa.h"
#include "botan/rsa.h"
#include "botan/elgamal.h"
#include "botan/numthry.h"
#include "botan/pow_mod.h"
#include "botan/kdf.h"
#include <botan/data_src.h>
#include <botan/scan_name.h>
#include <keto/crypto/Containers.hpp>
#include <botan/prf_x942.h>


namespace keto {
namespace crypto {

KeyBuilder::KeyBuilder() {

}

KeyBuilder::~KeyBuilder() {

}

KeyBuilder& KeyBuilder::addBytes(
        const std::vector<uint8_t>& bytes) {
    Botan::BigInt bigInt = Botan::BigInt(bytes);
    if (bigInt.is_negative()) {
        KETO_LOG_DEBUG <<"Big int is negative";
    }
    this->privateKeys.push_back(bigInt);

    return *this;
}

KeyBuilder& KeyBuilder::addPrivateKey(
        const std::shared_ptr<Botan::Private_Key>& bytes) {
    if (bytes->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PrivateKey> rsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PrivateKey>(bytes);
        this->privateKeys.push_back(rsaPrivateKeyPtr->get_d());
    } else if (bytes->algo_name() == "DSA") {
        std::shared_ptr<Botan::DSA_PrivateKey> dsaPrivateKeyPtr = std::dynamic_pointer_cast<Botan::DSA_PrivateKey>(bytes);
        this->privateKeys.push_back(dsaPrivateKeyPtr->get_x());
    } else if (bytes->algo_name() == "DH") {
        std::shared_ptr<Botan::DH_PrivateKey> dhPrivateKeyPtr = std::dynamic_pointer_cast<Botan::DH_PrivateKey>(bytes);
        this->privateKeys.push_back(dhPrivateKeyPtr->get_x());
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat());
    }
    return *this;
}

KeyBuilder& KeyBuilder::addPublicKey(
        const std::shared_ptr<Botan::Public_Key>& bytes) {
    if (bytes->algo_name() == "RSA") {
        std::shared_ptr<Botan::RSA_PublicKey> rsaPublicKeyPtr = std::dynamic_pointer_cast<Botan::RSA_PublicKey>(bytes);
        this->privateKeys.push_back(rsaPublicKeyPtr->get_n());
    } else if (bytes->algo_name() == "DSA") {
        std::shared_ptr<Botan::DSA_PublicKey> dsaPublicKeyPtr = std::dynamic_pointer_cast<Botan::DSA_PublicKey>(bytes);
        this->privateKeys.push_back(dsaPublicKeyPtr->get_y());
    } else if (bytes->algo_name() == "DH") {
        std::shared_ptr<Botan::DH_PublicKey> dhPublicKeyPtr = std::dynamic_pointer_cast<Botan::DH_PublicKey>(bytes);
        this->privateKeys.push_back(dhPublicKeyPtr->get_y());
    } else {
        BOOST_THROW_EXCEPTION(keto::crypto::UnsupportKeyFormat());
    }

    return *this;
}


std::shared_ptr<Botan::Private_Key> KeyBuilder::getPrivateKey() {
    if (!this->privateKeyPtr) {

        if (this->privateKeys.size() < 2) {
            BOOST_THROW_EXCEPTION(keto::crypto::InsufficiateKeyDataException());
        }

        // derive a private key using the dh logarithmic approach.
        Botan::DL_Group grp("modp/ietf/2048");
        Botan::BigInt currentKey = grp.get_q();
        for (int index = 0; index < this->privateKeys.size(); index++) {
            Botan::Power_Mod power_mod(grp.get_p());
            power_mod.set_base(currentKey);
            power_mod.set_exponent(this->privateKeys[index]);

            currentKey = power_mod.execute();
        }

        std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
        this->privateKeyPtr = std::shared_ptr<Botan::Private_Key>(new Botan::ElGamal_PrivateKey(*generator, grp,
                                                                                           currentKey));
    }
    return this->privateKeyPtr;
}

std::shared_ptr<Botan::Public_Key> KeyBuilder::getPublicKey() {
    return std::shared_ptr<Botan::Public_Key>(
            Botan::X509::load_key(this->privateKeyPtr->public_key_bits()));

}



}
}
