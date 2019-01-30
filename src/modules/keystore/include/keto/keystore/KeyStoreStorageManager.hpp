//
// Created by Brett Chaldecott on 2018/11/27.
//

#ifndef KETO_KEYSTORESTORAGEMANAGER_HPP
#define KETO_KEYSTORESTORAGEMANAGER_HPP

#include <string>
#include <memory>


#include "keto/key_store_db/KeyStoreDB.hpp"

#include "keto/event/Event.hpp"
#include "keto/crypto/KeyLoader.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/keystore/KeyStoreEntry.hpp"

#include "keto/memory_vault_session/MemoryVaultSessionKeyWrapper.hpp"


namespace keto {
namespace keystore {

class KeyStoreStorageManager;
typedef std::shared_ptr<KeyStoreStorageManager> KeyStoreStorageManagerPtr;

class KeyStoreStorageManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    KeyStoreStorageManager();
    KeyStoreStorageManager(const KeyStoreStorageManager& orig) = delete;
    virtual ~KeyStoreStorageManager();


    static KeyStoreStorageManagerPtr init();
    static void fin();
    static KeyStoreStorageManagerPtr getInstance();



    // init the store
    void initSession();
    void clearSession();


    // get the key loader
    std::shared_ptr<keto::crypto::KeyLoader> getKeyLoader();

private:
    keto::key_store_db::KeyStoreDBPtr keyStoreDBPtr;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;

};


}
}


#endif //KETO_KEYSTORESTORAGEMANAGER_HPP
