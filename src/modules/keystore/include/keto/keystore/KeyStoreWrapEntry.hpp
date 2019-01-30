//
// Created by Brett Chaldecott on 2019/01/27.
//

#ifndef KETO_KEYSTOREINDEXENTRY_HPP
#define KETO_KEYSTOREINDEXENTRY_HPP

#include <string>
#include <memory>
#include <map>
#include <vector>

#include <nlohmann/json.hpp>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/rsa.h>
#include <botan/auto_rng.h>

#include "KeyStore.pb.h"

#include "keto/keystore/Constants.hpp"
#include "keto/memory_vault_session/MemoryVaultSessionKeyWrapper.hpp"

namespace keto {
namespace keystore {

class KeyStoreStorageManager;

class KeyStoreWrapEntry;
typedef std::shared_ptr<KeyStoreWrapEntry> KeyStoreWrapEntryPtr;

class KeyStoreWrapEntry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    KeyStoreWrapEntry(const keto::proto::NetworkKey& networkKey);
    KeyStoreWrapEntry(const nlohmann::json& jsonEntry);
    KeyStoreWrapEntry(const KeyStoreWrapEntry& orig) = delete;
    virtual ~KeyStoreWrapEntry();

    std::vector<uint8_t> getHash();

    bool getActive();
    KeyStoreWrapEntry& setActive(bool active);

    nlohmann::json getJson();

    void setKey(const keto::proto::NetworkKey& networkKey);
    keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr getDerivedKey();


private:
    std::vector<uint8_t> hash;
    bool active;
    keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr derivedKey;

};


}
}


#endif //KETO_KEYSTOREINDEXENTRY_HPP
