//
// Created by Brett Chaldecott on 2018/12/03.
//

#include "keto/crypto/KeyBuilder.hpp"
#include "keto/crypto/Exception.hpp"

#include "botan/dh.h"
#include "botan/dsa.h"


namespace keto {
namespace crypto {

KeyBuilder::KeyBuilder() {

}

KeyBuilder::~KeyBuilder() {

}

KeyBuilder& KeyBuilder::addBytes(
        const std::vector<uint8_t>& bytes) {
    this->privateKeys.push_back(Botan::BigInt(bytes));
    return *this;
}

KeyBuilder& KeyBuilder::addPrivateKey(
        const std::shared_ptr<Botan::Private_Key>& bytes) {
    this->privateKeys.push_back(Botan::BigInt(bytes->private_key_bits()));
    return *this;
}

KeyBuilder& KeyBuilder::addPublicKey(
        const std::shared_ptr<Botan::Public_Key>& bytes) {
    this->privateKeys.push_back(Botan::BigInt(bytes->public_key_bits()));
    return *this;
}


std::shared_ptr<Botan::Private_Key> KeyBuilder::getPrivateKey() {
    if (!this->privateKeyPtr) {
        if (this->privateKeys.size() != 3) {
            BOOST_THROW_EXCEPTION(keto::crypto::InsufficiateKeyDataException());
        }
        std::unique_ptr<Botan::DL_Group> group(new Botan::DL_Group(this->privateKeys[0],this->privateKeys[1],this->privateKeys[2]));
        std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
        this->privateKeyPtr =
                std::shared_ptr<Botan::Private_Key>(new Botan::DSA_PrivateKey(*rng, *group));
    }
    return this->privateKeyPtr;
}

std::shared_ptr<Botan::Public_Key> KeyBuilder::getPublicKey() {
    return std::shared_ptr<Botan::Public_Key>(
            Botan::X509::load_key(this->privateKeyPtr->public_key_bits()));

}


}
}
