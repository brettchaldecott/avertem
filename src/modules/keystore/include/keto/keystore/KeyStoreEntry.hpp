//
// Created by Brett Chaldecott on 2018/12/05.
//

#ifndef KETO_KEYSTOREENTRY_HPP
#define KETO_KEYSTOREENTRY_HPP

#include <string>
#include <memory>

#include <nlohmann/json.hpp>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/rsa.h>
#include <botan/auto_rng.h>

#include "keto/keystore/Constants.hpp"

namespace keto {
namespace keystore {

class KeyStoreEntry;
typedef std::shared_ptr<KeyStoreEntry> KeyStoreEntryPtr;

class KeyStoreEntry {
public:
    KeyStoreEntry();
    KeyStoreEntry(const std::string& jsonString);
    KeyStoreEntry(const KeyStoreEntry& orig) = default;
    virtual ~KeyStoreEntry();

    std::shared_ptr<Botan::Private_Key> getPrivateKey();
    std::shared_ptr<Botan::Public_Key> getPublicKey();

    std::string getJson();

private:
    std::shared_ptr<Botan::Private_Key> privateKey;
    std::shared_ptr<Botan::Public_Key> publicKey;

};


}
}


#endif //KETO_KEYSTOREENTRY_HPP
