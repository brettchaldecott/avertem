//
// Created by Brett Chaldecott on 2018/12/05.
//

#include "keto/keystore/KeyStoreEntry.hpp"
#include "keto/keystore/Constants.hpp"

#include "keto/crypto/Containers.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/crypto/HashGenerator.hpp"

#include <botan/hex.h>
#include <botan/pkcs8.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/x509_key.h>



namespace keto {
namespace keystore {

KeyStoreEntry::KeyStoreEntry() : active(true) {
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    privateKey = std::shared_ptr<Botan::Private_Key>(
            new Botan::RSA_PrivateKey(*generator, 2056));
    this->hash = keto::crypto::SecureVectorUtils().copyFromSecure(
            keto::crypto::HashGenerator().generateHash(
                    Botan::PKCS8::BER_encode(*privateKey)));
    publicKey = std::shared_ptr<Botan::Public_Key>(
            new Botan::RSA_PublicKey((Botan::RSA_PrivateKey&)*privateKey));


}

KeyStoreEntry::KeyStoreEntry(const std::string& jsonString) {

    nlohmann::json jsonKeys = nlohmann::json::parse(jsonString);

    this->hash = Botan::hex_decode(jsonKeys[Constants::KEY_HASH]);
    keto::crypto::SecureVector privateKeyBits =
            Botan::hex_decode_locked(jsonKeys[Constants::PRIVATE_KEY],true);
    std::vector<uint8_t> publicKeyBits =
            Botan::hex_decode(jsonKeys[Constants::PUBLIC_KEY],true);
    this->active = jsonKeys[Constants::KEY_ACTIVE];

    Botan::DataSource_Memory memoryPrivateKeyDatasource(privateKeyBits);
    this->privateKey =
            Botan::PKCS8::load_key(memoryPrivateKeyDatasource);
    this->publicKey = std::shared_ptr<Botan::Public_Key>(Botan::X509::load_key(publicKeyBits));
}

KeyStoreEntry::~KeyStoreEntry() {

}

std::vector<uint8_t> KeyStoreEntry::getHash() {
    return this->hash;
}

std::shared_ptr<Botan::Private_Key> KeyStoreEntry::getPrivateKey() {
    return this->privateKey;
}

std::shared_ptr<Botan::Public_Key> KeyStoreEntry::getPublicKey() {
    return this->publicKey;
}

void KeyStoreEntry::setActive(bool active) {
    this->active = active;
}

bool KeyStoreEntry::getActive() {
    return this->active;
}


std::string KeyStoreEntry::getJson() {
    keto::crypto::SecureVector privateKeyBits = Botan::PKCS8::BER_encode(*this->privateKey);
    std::vector<uint8_t> publicKeyBits = Botan::X509::BER_encode(*this->publicKey);
    nlohmann::json json = {
            {Constants::KEY_HASH, Botan::hex_encode(this->hash,true)},
            {Constants::PRIVATE_KEY, Botan::hex_encode(privateKeyBits,true)},
            {Constants::PUBLIC_KEY, Botan::hex_encode(publicKeyBits,true)},
            {Constants::KEY_ACTIVE, this->active}
    };
    return json.dump();
}



}
}