//
// Created by Brett Chaldecott on 2018/11/27.
//

#ifndef KETO_KEYSTORESTORAGEMANAGER_HPP
#define KETO_KEYSTORESTORAGEMANAGER_HPP

#include <string>
#include <memory>


#include "keto/key_store_db/KeyStoreDB.hpp"

#include "keto/crypto/KeyLoader.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/keystore/KeyStoreEntry.hpp"


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

    /**
     * Init the store
     */
    void initStore();
    /**
     * Unlock store
     */
    void unlockStore();

    /**
     * This method sets the derived key needed unlock the store.
     *
     * @param derivedKey
     */
    void setDerivedKey(std::shared_ptr<Botan::Private_Key> derivedKey);


    /**
     * This method returns the master reference.
     *
     * @return TRUE if this is the master
     */
    bool isMaster() const;

private:
    keto::key_store_db::KeyStoreDBPtr keyStoreDBPtr;
    bool master;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    std::shared_ptr<Botan::Private_Key> derivedKey;
    KeyStoreEntryPtr masterKeyStoreEntry;

};


}
}


#endif //KETO_KEYSTORESTORAGEMANAGER_HPP
