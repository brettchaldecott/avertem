//
// Created by Brett Chaldecott on 2018/12/05.
//

#include "keto/keystore/KeyStoreEntry.hpp"
#include "keto/keystore/Constants.hpp"

#include "keto/crypto/Containers.hpp"
#include <botan/hex.h>
#include <botan/pkcs8.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/x509_key.h>



namespace keto {
namespace keystore {

KeyStoreEntry::KeyStoreEntry() {
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    privateKey = std::shared_ptr<Botan::Private_Key>(
            new Botan::RSA_PrivateKey(*generator, 2056));
    publicKey = std::shared_ptr<Botan::Public_Key>(
            new Botan::RSA_PublicKey((Botan::RSA_PrivateKey&)*privateKey));
}

KeyStoreEntry::KeyStoreEntry(const std::string& jsonString) {

    nlohmann::json jsonKeys = nlohmann::json::parse(jsonString);

    keto::crypto::SecureVector privateKeyBits =
            Botan::hex_decode_locked(jsonKeys[Constants::PRIVATE_KEY],true);
    std::vector<uint8_t> publicKeyBits =
            Botan::hex_decode(jsonKeys[Constants::PUBLIC_KEY],true);

    Botan::DataSource_Memory memoryPrivateKeyDatasource(privateKeyBits);
    this->privateKey =
            Botan::PKCS8::load_key(memoryPrivateKeyDatasource);
    this->publicKey = std::shared_ptr<Botan::Public_Key>(Botan::X509::load_key(publicKeyBits));
}

KeyStoreEntry::~KeyStoreEntry() {

}

std::shared_ptr<Botan::Private_Key> KeyStoreEntry::getPrivateKey() {
    return this->privateKey;
}

std::shared_ptr<Botan::Public_Key> KeyStoreEntry::getPublicKey() {
    return this->publicKey;
}

std::string KeyStoreEntry::getJson() {
    keto::crypto::SecureVector privateKeyBits = Botan::PKCS8::BER_encode(*this->privateKey);
    std::vector<uint8_t> publicKeyBits = Botan::X509::BER_encode(*this->publicKey);
    nlohmann::json json = {
            {Constants::PRIVATE_KEY, Botan::hex_encode(privateKeyBits,true)},
            {Constants::PUBLIC_KEY, Botan::hex_encode(publicKeyBits,true)}
    };
    return json.dump();
}



}
}