//
// Created by Brett Chaldecott on 2019/01/27.
//

#ifndef KETO_KEYSTOREINDEXMANAGER_HPP
#define KETO_KEYSTOREINDEXMANAGER_HPP

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

#include "keto/event/Event.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/keystore/KeyStoreWrapEntry.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"



namespace keto {
namespace keystore {

class KeyStoreWrapIndexManager;
typedef std::shared_ptr<KeyStoreWrapIndexManager> KeyStoreKeyIndexManagerPtr;

class KeyStoreWrapIndexManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    // constructors
    KeyStoreWrapIndexManager();
    KeyStoreWrapIndexManager(const KeyStoreWrapIndexManager& orig) = delete;
    virtual ~KeyStoreWrapIndexManager();

    // instance methods
    static KeyStoreKeyIndexManagerPtr init();
    static void fin();
    static KeyStoreKeyIndexManagerPtr getInstance();

    // init the store
    void initSession();
    void clearSession();

    // set the keys
    void setMasterKey(const keto::rpc_protocol::NetworkKeysHelper& event);
    void setWrappingKeys(const keto::rpc_protocol::NetworkKeysHelper& event);

protected:
    int getNumberOfKeys();
    keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr getKey(int index);

private:
    keto::key_store_db::KeyStoreDBPtr keyStoreDBPtr;
    keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr derivedKey;
    std::vector<std::vector<uint8_t>> index;
    std::map<std::vector<uint8_t>,KeyStoreWrapEntryPtr> networkKeys;

    void loadWrapperIndex();
    void setWrapperIndex();

};


}
}



#endif //KETO_KEYSTOREINDEXENTRY_HPP
